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

const int width = 800;
const int height = 800;
const int depth = 255;

Model* model=NULL;
float* zbuffer = NULL;
//Vec3f light_dir(0, 0, -1);
Vec3f light_dir = Vec3f(1, -1, 1).normalize();

//绘制线段
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
		//误差变量为我们提供了从当前（x，y）像素到最佳直线的距离。
		//每当误差大于一个像素时，我们都会将 y 增加（或减少）一个，并将误差减少一个。
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

//void lookat(Vec3f eye, Vec3f center, Vec3f up)
//{
//	Vec3f z = (eye - center).normalize();
//	Vec3f x = up ^ z;
//	Vec3f y = z ^ x;
//	x.normalize();
//	y.normalize();
//	Matrix Minv = Matrix::identity();
//	Matrix Tr = Matrix::identity();
//	for (int i = 0; i < 3; i++)
//	{
//		Minv[0][i] = x[i];
//		Minv[1][i] = y[i];
//		Minv[2][i] = z[i];
//		Tr[i][3] = -eye[i];
//	}
//	
//}

//计算给定三角形中点P的坐标
Vec3f barycentric(Vec3f* pts, Vec3f P)
{
	Vec3f u = Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - P.y);
	//防止后面除以u.z值时，u.z作为分母却为0；
	if (std::abs(u.z) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator

}
//计算uv坐标，进行适配处理
Vec3f world2screen(Vec3f v)
{
	////将原本限制在2的范围内的顶点数据重新放大到一定宽高的屏幕上，加1是为了将负数的顶点变为正数，再进行放大处理
	return Vec3f(int((v.x + 1.) * width / 2 + 0.5), int((v.y + 1.) * height / 2 + 0.5), v.z);
}

Vec2i computeUV(float u, float v, TGAImage& texture)
{
	return Vec2i(int(u * texture.get_width()), int(v * texture.get_height()));
}

void triangle(Vec3f* pts,Vec3f* texture_coords ,Vec3f* normal_coords,float* zbuffer, TGAImage& image,TGAImage& texture,float uv_intensity)
{	 
	float intensity[3];
	for (int i = 0; i < 3; i++)
		{
			normal_coords[i].normalize();
			intensity[i] = normal_coords[i] * light_dir;
			if (intensity[i] < 0.f) intensity[i] = 0.f;
	}
	
	float finalintensity=uv_intensity;
	//保证获取到最小盒子，所以初始化为最大
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	//clamp保证计算bboxmax的时候获取到屏幕大小
	//Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
	//获取包围盒
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
			finalintensity = 0.0f;
			float u = 0.0f;
			float v = 0.0f;
			TGAColor color;
			//对z值进行插值,bc_screen[0]=1-u-v,bc_screen[1]=u,bc_screen[2]=v,分别乘以三个点的z值进行插值
			for (int i = 0; i < 3; i++)
			{
				P.z += pts[i].z * bc_screen[i];
			}

			//对uv进行插值
			for (int i = 0; i < 3; i++)
			{
				u += texture_coords[i].x * bc_screen[i];
				v += texture_coords[i].y * bc_screen[i];
			}
			//计算uv在图片上的坐标。
			Vec2i middle_color = computeUV(u, v, texture);
			color = texture.get(middle_color.x,middle_color.y);
			color.b = color.b * finalintensity;
			color.g = color.g * finalintensity;
			color.r = color.r * finalintensity;

			//gouraud阴影计算
			//三个顶点的阴影法线
			//for (int i = 0; i < 3; i++) intensity_array[i] = gouraud_coords[i] * light_dir;
			////进行插值
			for (int i = 0; i < 3; i++)
			{
				finalintensity += intensity[i] * bc_screen[i];
			}
			/*color = white;
			color.b = color.b * finalintensity;
			color.g = color.g * finalintensity;
			color.r = color.r * finalintensity;*/

			if (zbuffer[int(P.x + P.y * width)] < P.z)
			{
				zbuffer[int(P.x + P.y * width)] = P.z;
				image.set(P.x, P.y, TGAColor(255 * finalintensity, 255 * finalintensity, 255 * finalintensity,255));
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
	//纹理颜色
	TGAImage texture;
	int creat_texture=texture.read_tga_file("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\african_head\\african_head_diffuse.tga");
	texture.flip_vertically();
	TGAColor texture_color;
	TGAImage image(width, height, TGAImage::RGB);
	zbuffer = new float[width * height];
	//std::numeric_limits<T>::min()/max() 函数可用于获取由数字类型T表示的最小、最大有限值。
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
	//model->nfaces()返回三角形的数量
	for (int i = 0; i < model->nfaces(); i++) {
		//face是三角形面
		std::vector<int> face = model->face(i);
		//保存转换后的三角形顶点数据
		Vec3f pts[3];
		//保存未转换的三角形顶点数据
		Vec3f init_coords[3];
		//保存相应顶点的颜色数据
		Vec3f texture_coords[3];
		//保存gouraud阴影法线数据
		Vec3f normal_coords[3];
		for (int j = 0; j < 3; j++)
		{
			pts[j] = world2screen(model->vert(face[j*3]));
			texture_coords[j] =model->texture(face[int(j*3+1)]);
			init_coords[j] = model->vert(face[j*3]);
			normal_coords[j] = model->Gour(face[j*3+2]);

		}
		//叉乘获得三角形法线方向
		/*Vec3f n = (init_coords[2] - init_coords[0] ^ (init_coords[1] - init_coords[0]));
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0)
		{
			triangle(pts,texture_coords, normal_coords, zbuffer, image, texture, light_dir, intensity);
		}*/
		/*for (int i = 0; i < 3; i++)
		{
			normal_coords[i].normalize();
			intensity[i] = normal_coords[i] * light_dir;
			if (intensity[i] < 0.f) intensity[i] = 0.f;
		}*/
		//void triangle(Vec3f * pts, Vec3f * texture_coords, Vec3f * gouraud_coords, float* zbuffer, TGAImage & image, TGAImage & texture, float* intensity, float uv_intensity)
		triangle(pts, texture_coords, normal_coords,zbuffer, image, texture,0.f);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	delete model;
	return 0;
}
