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
	mat<4, 4, float> uniform_M;
	mat<4, 4, float> uniform_MIT;
	//表示1：//对顶点坐标体系进行变换，以及获取到三个顶点的uv和光照强度（光照强度等于法线乘以光照向量）

	//表示2：光照强度不再使用三个顶点的光照强度值进行插值
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//表示1：
		//根据法线向量计算光照强度
		//varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//计算uv,按列填入数字，每一列都是一个顶点的uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//对顶点引入齐次方程
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//视图变换
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//表示1：根据顶点获取到的光照强度和uv进行插值，然后通过uv坐标获取图片的颜色，最后乘以光照强度就是最后的颜色

	//如果我们有一个模型，其法向量由艺术家给出，并且该模型用仿射映射进行变换，则法向量将使用映射进行变换，等于原始映射矩阵的反矩阵的转置
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		//表示1：
		//float intensity = varying_intensity * bar;
		Vec2f uv = varying_uv * bar;
		//法向量的变换
		Vec3f n=proj<3>(uniform_MIT*embed<4>(model->normal(uv))).normalize();
		//光照向量坐标体系变换
		Vec3f L = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		//如果 n 和 l 被归一化，则 r = 2n<n，l> - l
		Vec3f r = (n * (n * L * 2.f) - L).normalize();
		float diffuse = std::max(0.f, n * L);
		//pow函数返回x的y次方
		float specular = pow(std::max(r.z, 0.f), model->specular(uv));
		//选择读取哪个纹理图片
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + color[i] * (diffuse + 0.6 * specular), 255);
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
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT=(Projection*ModelView).invert_transpose();
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
