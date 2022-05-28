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
	Viewport[2][3] = depth/2.f;
	Viewport[0][0] = w / 2.f;
	Viewport[1][1] = h / 2.f;
	Viewport[2][2] = depth/2.f;
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



void triangle(mat<4,3,float>& clipc, IShader& shader, TGAImage& image, float *zbuffer)
{
	//�԰������������ζ������������ţ�Ȼ��ת�ã��������
	mat<3, 4, float> pts = (Viewport * clipc).transpose();
	//��ȡ���������x/w��y/w��������������������
	mat<3, 2, float> pts2;
	for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	//for (int i = 0; i < 3; i++)
	//{
	//	for (int j = 0; j < 2; j++)
	//	{
	//		//�������һ����������Ϊ��������η���
	//		bboxmin[j] = std::min(bboxmin[j], pts2[i][j]);
	//		bboxmax[j] = std::max(bboxmax[j], pts2[i][j]);
	//	}
	//}
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			//��ʽ1��
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts2[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
			//��ʽ2��
			//bboxmin[j] = std::min(bboxmin[j], pts2[i][j]);
			//bboxmax[j] = std::max(bboxmax[j], pts2[i][j]);
		}
	}
	Vec2i P;
	TGAColor color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
//////////////////////////��ʽ1///////////////////////////////////////////////////////////////////////////////////////////////////////////
			////������������
			//Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
			//Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
			//bc_clip = bc_clip / (bc_clip.x, +bc_clip.y + bc_clip.z);
			////��zֵ���в�ֵ
			//float z = pts[0][2] * bc_screen.x + pts[1][2] * bc_screen.y + pts[2][2] * bc_screen.z;
			////����η��������w���в�ֵ
			//float w = pts[0][3] * bc_screen.x + pts[1][3] * bc_screen.y + pts[2][3] * bc_screen.z;
			////��ȡ��Ȳ�ֵ������������0-255,��0.5Ӧ����Ϊ��ȷ������������
			////int frag_depth = std::max(0, std::min(255, int(z / w + 0.5)));
			////�ſ��������
			//float frag_depth = z/w;
			
//////////////////////////��ʽ2///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//����������
			Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
			//ΪʲôҪ����w��bc_clip��������ʲô���Ƶ�������
			Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
			float frag_depth = clipc[2] * bc_clip;
			
			//zbuffer.get(P.x, P.y)[0] ���� frag_depth����ζ�ŵ�ǰzbuffer��������������������ѭ��
			//zbuffer[P.x+P.y*image.get_width()]>frag_depth
			//zbuffer.get(P.x, P.y)[0]>frag_depth
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z<0 || zbuffer[int(P.x+P.y*image.get_width())]>frag_depth) continue;
			//��ȡ��ɫ
			//bool discard = shader.fragment(c, color);
			Vec3f gl_fragcoord(P.x, P.y, frag_depth);
			bool discard = shader.fragment(gl_fragcoord, bc_clip, color);
			if (!discard)
			{
				//����frag_depth��Χ���Ӷ���������
				/*if (frag_depth > max) max = frag_depth;
				if (frag_depth < min) min = frag_depth;*/
				//min=431��max=1871��ϵ��Ϊ255/(1871-431)=0.1777;
				/*int zbuffer_value = std::max(0, std::min(255, int((frag_depth - 432) * 255/(1871-431))));
				zbuffer.set(P.x, P.y, TGAColor(zbuffer_value));*/
				zbuffer[int(P.x + P.y * image.get_width())] = frag_depth;
				image.set(P.x, P.y, color);
				
			}
		}
	}
}


