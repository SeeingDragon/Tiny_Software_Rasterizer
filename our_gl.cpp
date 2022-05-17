#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"


Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {};

//��������ת�����Ŵ���Ļ���꣬ͬ����world2screen();
void viewport(int x, int y, int w, int h)
{
	Viewport = Matrix::identity();
	Viewport[0][3] = x + w / 2.f;
	Viewport[1][3] = y + h / 2.f;
	Viewport[2][3] = depth / 2.f;
	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = depth / 2.f;
}

//ͶӰ����
void projection(float coeff)
{
	Projection = Matrix::identity();
	Projection[3][2] = coeff;
}

//������ϵ�任
void lookat(Vec3f eye, Vec3f center, Vec3f up)
{
	Vec3f z = (eye - center).normalize();
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z, x).normalize();
	ModelView = Matrix::identity();
	for (int i = 0; i < 3; i++)
	{
		ModelView[0][i] = x[i];
		ModelView[1][i] = y[i];
		ModelView[2][i] = z[i];
		ModelView[i][3] = -center[i];
	}
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P)
{
	Vec3f s[2];
	for (int i = 2; i--;)
	{
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	Vec3f u = cross(s[0], s[1]);
	//��ֹ�������u.zֵʱ��u.z��Ϊ��ĸȴΪ0
	if (std::abs(u[2]) > 1e-2)
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1);
}



void triangle(Vec4f* pts, IShader& shader, TGAImage& image,TGAImage& zbuffer, float *depthbuffer,int &min,int &max)
{
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			//�������һ����������Ϊ��������η���
			bboxmin[j] = std::min(bboxmin[j], pts[i][j] / pts[i][3]);
			bboxmax[j] = std::max(bboxmax[j], pts[i][j] / pts[i][3]);
		}
	}
	Vec2i P;
	TGAColor color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
			//proj<len>������ȡ����ǰ��len�����֣������������һ������Ϊ��������η���
			Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
			//��zֵ���в�ֵ
			float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
			//����η��������w���в�ֵ
			float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
			//��ȡ��Ȳ�ֵ������������0-255,��0.5Ӧ����Ϊ��ȷ������������
			//int frag_depth = std::max(0, std::min(255, int(z / w + 0.5)));
			//�ſ��������
			int frag_depth = z / w;
			//zbuffer.get(P.x, P.y)[0] ���� frag_depth����ζ�ŵ�ǰzbuffer��������������������ѭ��
			//zbuffer[P.x+P.y*image.get_width()]>frag_depth
			//zbuffer.get(P.x, P.y)[0]>frag_depth
			if (c.x < 0 || c.y < 0 || c.z<0 || depthbuffer[int(P.x+P.y*image.get_width())]>frag_depth) continue;
			//��ȡ��ɫ
			bool discard = shader.fragment(c, color);
			if (!discard)
			{
				//����frag_depth��Χ���Ӷ���������
				if (frag_depth > max) max = frag_depth;
				if (frag_depth < min) min = frag_depth;
				//min=431��max=1871��ϵ��Ϊ255/(1871-431)=0.1777;
				int zbuffer_value = std::max(0, std::min(255, int((frag_depth - 432) * 255/(1871-431))));
				zbuffer.set(P.x, P.y, TGAColor(zbuffer_value));
				depthbuffer[int(P.x + P.y * image.get_width())] = frag_depth;
				image.set(P.x, P.y, color);
			}
		}
	}
}

