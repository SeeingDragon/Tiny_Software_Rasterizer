#include <vector> 
#include <iostream> 
#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"


Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct GouraudShader :public IShader
{
	//顶点着色器写入，片段着色器读取。
	Vec3f varying_intensity;
	mat<2, 3, float> varying_uv;
	//返回顶点并且写入varying_intensity的xyz
	virtual Vec4f vertex(int iface, int nthvert)
	{
		
		//根据法线向量计算光照强度
		Vec3f gouraud = model->normal(iface, nthvert);
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//计算uv,按列填入数字，每一列都是一个顶点的uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//对顶点引入齐次方程
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//视图变换
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		float intensity = varying_intensity * bar;
		Vec2f uv = varying_uv * bar;
		color = model->diffuse(uv);
		return false;
	}
};

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
	lookat(eye, center, up);
	//为什么采用3/4width和height
	viewport(width / 8, height / 8, width*3/4 , height*3/4);
	//norm函数return std::sqrt(x * x + y * y + z * z)
	projection(-1.f / (eye - center).norm());
	light_dir.normalize();
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	GouraudShader shader;
	
	//model->nfaces()返回三角形的数量
	for (int i = 0; i < model->nfaces(); i++) {
		//保存三个顶点
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++)
		{
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}
	
	image.flip_vertically();
	zbuffer.flip_vertically();
	image.write_tga_file("output.tga");
	zbuffer.write_tga_file("zbuffer.tga");

	delete model;
	return 0;
}
