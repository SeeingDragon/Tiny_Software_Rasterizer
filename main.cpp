#include <vector> 
#include <iostream> 
#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

const int width = 800;
const int height = 800;
const int depth = 255;

Model* model=NULL;
float* zbuffer = NULL;
//����������Ҫ�Ĺ���
//Vec3f light_dir(0, 0, -1);
Vec3f light_dir = Vec3f(1, -1, 1).normalize();
//Vec3f light_dir(1, -1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct GouraudShader:public IShader
{
	//������ɫ��д�룬Ƭ����ɫ����ȡ��
	Vec3f varying_intensity;
	virtual Vec4f vertex(int iface, int nthvert)
	{

	}
};

//�����߶�
void line(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
	int x0 = v0.x;
	int y0 = v0.y;
	int x1 = v1.x;
	int y1 = v1.y;
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	float derror2 = std::abs(dy) * 2;
	float error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		//������Ϊ�����ṩ�˴ӵ�ǰ��x��y�����ص����ֱ�ߵľ��롣
		//ÿ��������һ������ʱ�����Ƕ��Ὣ y ���ӣ�����٣�һ��������������һ����
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2.;
		}
	}
}

void rasterize(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color, int ybuffer[])
{
	if (p0.x > p1.x)
	{
		std::swap(p0, p1);
	}
	for (int x = p0.x; x <= p1.x; x++)
	{
		float t = (x - p0.x) / (float)(p1.x - p0.x);
		int y = p0.y * (1. - t) + p1.y * t;
		if (ybuffer[x] < y)
		{
			ybuffer[x] = y;
			for (int j = 0; j < 16; j++)
			{
				image.set(x, j, color);
			}

		}
	}
}

//��������������е�P������
Vec3f barycentric(Vec3f* pts, Vec3f P)
{
	Vec3f u = cross(Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - P.x),Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - P.y));
	//��ֹ�������u.zֵʱ��u.z��Ϊ��ĸȴΪ0��
	if (std::abs(u.z) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator

}
//����uv���꣬�������䴦��
Vec3f world2screen(Vec3f v)
{
	////��ԭ��������2�ķ�Χ�ڵĶ����������·Ŵ�һ����ߵ���Ļ�ϣ���1��Ϊ�˽������Ķ����Ϊ�������ٽ��зŴ���
	return Vec3f(int((v.x + 1.) * width / 2 + 0.5), int((v.y + 1.) * height / 2 + 0.5), v.z);
}

Vec2i computeUV(float u, float v, TGAImage& texture)
{
	return Vec2i(int(u * texture.get_width()), int(v * texture.get_height()));
}

void triangle(Vec3f* pts,Vec2f* texture_coords ,Vec3f* normal_coords,float* zbuffer, TGAImage& image,TGAImage& texture,float uv_intensity)
{	 
	float intensity[3];
	for (int i = 0; i < 3; i++)
		{
			normal_coords[i].normalize();
			intensity[i] = normal_coords[i] * light_dir;
			if (intensity[i] < 0.f) intensity[i] = 0.f;
	}
	
	float finalintensity=uv_intensity;
	//��֤��ȡ����С���ӣ����Գ�ʼ��Ϊ���
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	//clamp��֤����bboxmax��ʱ���ȡ����Ļ��С
	//Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
	//��ȡ��Χ��
	for (int i = 0; i < 3; i++)
	{
		bboxmin.x = std::max(0.f, std::min(bboxmin.x, pts[i].x));
		bboxmin.y = std::max(0.f, std::min(bboxmin.y, pts[i].y));

		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));

	}
	Vec3f P;
	
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			P.z = 0;
			//��������ʱ��Ҫ��finalintensity = 0.0f;ע�͵�
			finalintensity = 0.0f;
			float u = 0.0f;
			float v = 0.0f;
			TGAColor color;
			//��zֵ���в�ֵ,bc_screen[0]=1-u-v,bc_screen[1]=u,bc_screen[2]=v,�ֱ�����������zֵ���в�ֵ
			for (int i = 0; i < 3; i++)
			{
				P.z += pts[i].z * bc_screen[i];
			}

			//��uv���в�ֵ
			for (int i = 0; i < 3; i++)
			{
				u += texture_coords[i].x * bc_screen[i];
				v += texture_coords[i].y * bc_screen[i];
			}
			//����uv��ͼƬ�ϵ����ꡣ
			Vec2i middle_color = computeUV(u, v, texture);
			//�õ�������ɫ
			color = texture.get(middle_color.x,middle_color.y);
			color.b = color.b * uv_intensity;
			color.g = color.g * uv_intensity;
			color.r = color.r * uv_intensity;

			//gouraud��Ӱ����
			//���в�ֵ
			for (int i = 0; i < 3; i++)
			{
				finalintensity += intensity[i] * bc_screen[i];
			}
			if (zbuffer[int(P.x + P.y * width)] < P.z)
			{
				zbuffer[int(P.x + P.y * width)] = P.z;
				//����������color
				//TGAColor(255 * finalintensity, 255 * finalintensity, 255 * finalintensity,255)
				//color
				image.set(P.x, P.y, TGAColor(255 * finalintensity, 255 * finalintensity, 255 * finalintensity, 255));
			}
		}
	}
}


int main(int argc, char** argv)
{
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\african_head\\african_head.obj");
	}
	float intensity[3];
	//������ɫ
	TGAImage texture;
	int creat_texture=texture.read_tga_file("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\african_head\\african_head_diffuse.tga");
	texture.flip_vertically();
	TGAColor texture_color;
	TGAImage image(width, height, TGAImage::RGB);
	zbuffer = new float[width * height];
	//std::numeric_limits<T>::min()/max() ���������ڻ�ȡ����������T��ʾ����С���������ֵ��
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
	//model->nfaces()���������ε�����
	for (int i = 0; i < model->nfaces(); i++) {
		//face����������
		std::vector<int> face = model->face(i);
		//����ת����������ζ�������
		Vec3f pts[3];
		//����δת���������ζ�������
		Vec3f init_coords[3];
		//������Ӧ�������ɫ����
		Vec2f texture_coords[3];
		//����gouraud��Ӱ��������
		Vec3f normal_coords[3];
		for (int j = 0; j < 3; j++)
		{
			pts[j] = world2screen(model->vert(face[j]));
			texture_coords[j] = model->uv(i,j);
			init_coords[j] = model->vert(face[j]);
			normal_coords[j] = model->normal(i,j);

		}
		//��˻�������η��߷���
		Vec3f n = cross(init_coords[2] - init_coords[0],init_coords[1] - init_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0)
		{
			//����uv����������ɫ
			//triangle(pts,texture_coords, normal_coords, zbuffer, image, texture,intensity);
		}
		//����Gouraud��Ӱ
		triangle(pts, texture_coords, normal_coords,zbuffer, image, texture,0.f);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	delete model;
	return 0;
}
