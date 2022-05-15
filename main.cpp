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
	//表示1：//对顶点坐标体系进行变换，以及获取到三个顶点的uv和光照强度（光照强度等于法线乘以光照向量）

	virtual Vec4f vertex(int iface, int nthvert)
	{
		//根据法线向量计算光照强度
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//对顶点引入齐次方程
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//视图变换
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//表示1：根据顶点获取到的光照强度和uv进行插值，然后通过uv坐标获取图片的颜色，最后乘以光照强度就是最后的颜色

	//如果我们有一个模型，其法向量由艺术家给出，并且该模型用仿射映射进行变换，则法向量将使用映射进行变换，等于原始映射矩阵的反矩阵的转置
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		float intensity = varying_intensity * bar;
		color =TGAColor(255 * intensity, 255 * intensity, 255 * intensity,255);
		return false;
	}
};

struct TextureShader1 :public IShader
{
	//顶点着色器写入，片段着色器读取。
	Vec3f varying_intensity;
	mat<2, 3, float> varying_uv;
	//表示1：//对顶点坐标体系进行变换，以及获取到三个顶点的uv和光照强度（光照强度等于法线乘以光照向量）
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//表示1：
		//根据法线向量计算光照强度
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//计算uv,按列填入数字，每一列都是一个顶点的uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//对顶点引入齐次方程
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//视图变换
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//表示1：根据顶点获取到的光照强度和uv进行插值，然后通过uv坐标获取图片的颜色，最后乘以光照强度就是最后的颜色
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		//表示1：
		float intensity = varying_intensity * bar;
		Vec2f uv = varying_uv * bar;
		//选择读取哪个纹理图片
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) color[i] = color[i] * intensity;
		return false;
	}
};

//表示2：光照强度不再使用三个顶点的光照强度值进行插值
struct TextureShader2 :public IShader
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
		//计算uv,按列填入数字，每一列都是一个顶点的uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//对顶点引入齐次方程
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//视图变换
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//如果我们有一个模型，其法向量由艺术家给出，并且该模型用仿射映射进行变换，则法向量将使用映射进行变换，等于原始映射矩阵的反矩阵的转置
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
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

//Phong着色
struct PhongShader :public IShader {
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_nrm;

	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface, nthvert), 0.f)));
		Vec4f gl_vertex = Viewport * Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		return gl_vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		Vec3f bn = (varying_nrm * bar).normalize();
		Vec2f uv = varying_uv * bar;
		float diffuse =std::max(0.f, bn * proj<3>(Projection*ModelView * embed<4>(light_dir)).normalize());
		color = model->diffuse(uv)*diffuse;
		//for (int i = 0; i < 3; i++) color[i] = color[i] * diffuse;
		return false;
	}
};

struct Shader :public IShader
{
	//存储uv坐标
	mat<2, 3, float> varying_uv;
	//把顶点按列存入
	mat<4, 3, float> varying_tri;
	//存储原始法向量
	mat<3, 3, float> varying_nrm;
	//存储去掉齐次方程的顶点数据
	mat<3,3,float> ndc_tri;
	
	virtual Vec4f vertex(int iface,int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface, nthvert))));
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Viewport * Projection * ModelView * gl_vertex;
		varying_tri.set_col(nthvert, gl_vertex);
		ndc_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));
		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		//对原法向量插值
		Vec3f bn = (varying_nrm * bar).normalize();
		//对原uv插值
		Vec2f uv = varying_uv * bar;

		mat<3, 3, float> A;
		//第一个顶点减掉第零个顶点，存入A的第零行
		//按行存储
		A[0] = ndc_tri.col(1) - ndc_tri.col(0);
		A[1] = ndc_tri.col(2) - ndc_tri.col(0);
		A[2] = bn;

		//求逆矩阵
		mat<3, 3, float> AI = A.invert();
		Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0],varying_uv[0][2] - varying_uv[0][0],0.f);
		Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0],0.f);
		
		//按列存储
		mat<3, 3, float> B;
		B.set_col(0, i.normalize());
		B.set_col(1, j.normalize());
		B.set_col(2, bn);
		
		//乘以矩阵B进行坐标基变换
		Vec3f n = (B * model->normal(uv)).normalize();
		//不能把光照放在这里算，因为这样每个像素都把光照算一次，累加效果变化太快，图象产生走样现象
		//light_dir = proj<3>((Projection * ModelView) * embed<4>(light_dir)).normalize();
		Vec3f r = n * (n * light_dir) * 2.f - light_dir;

		float diffuse = std::max(0.f, n*light_dir);
		float specular = pow(std::max(r.z,0.f),model->specular(uv));
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + color[i] * (diffuse + 0.6 * specular), 255.f);
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
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	//gouraud阴影着色
	//GouraudShader shader;

	//纹理着色
	//TextureShader1 shader;
	/*TextureShader2 shader;
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT=(Projection*ModelView).invert_transpose();*/

	light_dir = proj<3>((Projection * ModelView) * embed<4>(light_dir)).normalize();

	Shader shader;
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
