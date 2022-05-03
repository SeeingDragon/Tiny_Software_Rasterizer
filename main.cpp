#include <vector> 
#include <iostream> 
#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Model* model;
const int width = 800;
const int height = 800;

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
	Vec3f u = Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - P.y);
	//��ֹ�������u.zֵʱ��u.z��Ϊ��ĸȴΪ0��
	if (std::abs(u.z) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator

}

void triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color)
{
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
			//��zֵ���в�ֵ
			P.z += pts[0].z * bc_screen.x;
			P.z += pts[1].z * bc_screen.y;
			P.z += pts[2].z * bc_screen.z;

			if (zbuffer[int(P.x + P.y * width)] < P.z)
			{
				zbuffer[int(P.x + P.y * width)] = P.z;
				image.set(P.x, P.y, color);
			}
		}
	}
}
Vec3f world2screen(Vec3f v)
{
	////��ԭ��������2�ķ�Χ�ڵĶ����������·Ŵ�һ����ߵ���Ļ�ϣ���1��Ϊ�˽������Ķ����Ϊ�������ٽ��зŴ���
	return Vec3f(int((v.x + 1.) * width / 2 + 0.5), int((v.y + 1.) * height / 2 + 0.5), v.z);
}

int main(int argc, char** argv)
{
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\african_head.obj");
	}
	TGAImage image(width, height, TGAImage::RGB);
	float* zbuffer = new float[width * height];
	//std::numeric_limits<T>::min()/max() ���������ڻ�ȡ����������T��ʾ����С���������ֵ��
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
	//�������
	Vec3f light_dir(0, 0, -1);
	//model->nfaces()���������ε�����
	for (int i = 0; i < model->nfaces(); i++) {
		//face����������
		std::vector<int> face = model->face(i);
		//����ת����������ζ�������
		Vec3f pts[3];
		//����δת���������ζ�������
		Vec3f init_coords[3];
		for (int j = 0; j < 3; j++) 
		{
			pts[j] = world2screen(model->vert(face[j]));
			init_coords[j] = model->vert(face[j]);
		}
		//��˻�������η��߷���
		Vec3f n = (init_coords[2] - init_coords[0] ^ (init_coords[1] - init_coords[0]));
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0)
		{
			triangle(pts, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
		}
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	delete model;
	return 0;
}
