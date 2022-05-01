
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
const int width =800;
const int height = 800;

//绘制线段
void line(Vec2i v0,Vec2i v1, TGAImage& image, TGAColor color) {
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
	float derror2 = std::abs(dy)*2;
	float error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1;x++) {
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		//误差变量为我们提供了从当前（x，y）像素到最佳直线的距离。
		//每当误差大于一个像素时，我们都会将 y 增加（或减少）一个，并将误差减少一个。
		error2 += derror2;
		if (error2 >dx)
		{
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx*2.;
		}
	}
}


//计算给定三角形中点P的坐标
Vec3f barycentric(Vec2i *pts, Vec2i P)
{
	Vec3f u = Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - P.y);
	if (std::abs(u.z) < 1) return Vec3f(-1, 1, 1);
	return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);

}

void triangle(Vec2i* pts, TGAImage& image, TGAColor color)
{
	Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
	Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
	Vec2i bboxmax(0, 0);
	//clamp保证计算bboxmax的时候获取到屏幕大小
	//Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
	//获取包围盒
	for (int i = 0; i < 3; i++)
	{
		bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
		bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));

	}
	Vec2i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			image.set(P.x, P.y, color);
		}
	}
	////根据y值对三角形进行排序,t0<t1<t2
	//if (t0.y > t1.y) std::swap(t0, t1);
	//if (t0.y > t2.y) std::swap(t0, t2);
	//if (t1.y > t2.y) std::swap(t1, t2);
	////计算总的高度
	//int total_height = t2.y - t0.y;
	//for (int i = 0; i <= total_height; i++)
	//{
	//	bool second_half = i>(t1.y - t0.y) || t1.y==t0.y;
	//	//?切割高度(为什么要加1？？？)
	//	int segment_height =second_half ? t2.y - t1.y  :t1.y - t0.y;
	//	//求当前y到t0.y之间的距离与总高度之间的比值
	//	float alpha = (float)(i) / total_height;
	//	//求当前y到t0.y之间的距离与切割高度之间的比值
	//	float beta = (float)(i-(second_half ? t1.y-t0.y : 0)) / segment_height;
	//	//A线：t0-t2，B线：t0-t1
	//	Vec2i A = t0 + (t2 - t0) * alpha;
	//	Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
	//	if (A.x > B.x) std::swap(A, B);
	//	for (int j = A.x; j <= B.x; j++)
	//	{
	//		//注意y值要加上t0.y，因为此时的i值不是坐标系中的值
	//		image.set(j, t0.y+i, color);
	//	}
	//}
}

void man(Model* model, TGAImage& image, TGAColor color)
{
	//定义光照
	Vec3f light_dir(0, 0, -1);
	//model->nfaces()返回三角形的数量
	for (int i = 0; i < model->nfaces(); i++) {
		//face是三角形面
		std::vector<int> face = model->face(i);
		//保存转换后的三角形顶点数据
		Vec2i screen_coords[3];
		//保存未转换的三角形顶点数据
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			//world_coords是顶点
			Vec3f v = model->vert(face[j]);
			////将原本限制在2的范围内的顶点数据重新放大到一定宽高的屏幕上，加1是为了将负数的顶点变为正数，再进行放大处理。
			screen_coords[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
			world_coords[j] = v;
		}
		//叉乘获得三角形法线方向
		Vec3f n = (world_coords[2] - world_coords[0] ^ (world_coords[1] - world_coords[0]));
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0)
		{
			triangle(screen_coords, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
		}
		
	}
}

void rasterizer(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color, int ybuffer[])
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
			image.set(x, 0, color);
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
		model = new Model("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\african_head.obj");
	}
	TGAImage image(width, 16, TGAImage::RGB);
	//man(model,image, red);
	/*line(Vec2i(20, 34), Vec2i(744, 400), image, red);
	line(Vec2i(120, 434), Vec2i(444, 400),image,green);
	line(Vec2i(330, 463), Vec2i(594, 200), image, blue);

	line(Vec2i(10, 10), Vec2i(790, 10), image, white);*/
	int ybuffer[width];
	for (int i = 0; i < width; i++)
	{
		ybuffer[i] = std::numeric_limits<int>::min();
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	delete model;
	return 0;
}