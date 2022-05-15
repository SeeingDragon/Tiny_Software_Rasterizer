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
	//������ɫ��д�룬Ƭ����ɫ����ȡ��
	Vec3f varying_intensity;
	//��ʾ1��//�Զ���������ϵ���б任���Լ���ȡ�����������uv�͹���ǿ�ȣ�����ǿ�ȵ��ڷ��߳��Թ���������

	virtual Vec4f vertex(int iface, int nthvert)
	{
		//���ݷ��������������ǿ��
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//�Զ���������η���
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//��ͼ�任
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//��ʾ1�����ݶ����ȡ���Ĺ���ǿ�Ⱥ�uv���в�ֵ��Ȼ��ͨ��uv�����ȡͼƬ����ɫ�������Թ���ǿ�Ⱦ���������ɫ

	//���������һ��ģ�ͣ��䷨�����������Ҹ��������Ҹ�ģ���÷���ӳ����б任����������ʹ��ӳ����б任������ԭʼӳ�����ķ������ת��
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		float intensity = varying_intensity * bar;
		color =TGAColor(255 * intensity, 255 * intensity, 255 * intensity,255);
		return false;
	}
};

struct TextureShader1 :public IShader
{
	//������ɫ��д�룬Ƭ����ɫ����ȡ��
	Vec3f varying_intensity;
	mat<2, 3, float> varying_uv;
	//��ʾ1��//�Զ���������ϵ���б任���Լ���ȡ�����������uv�͹���ǿ�ȣ�����ǿ�ȵ��ڷ��߳��Թ���������
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//��ʾ1��
		//���ݷ��������������ǿ��
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//����uv,�����������֣�ÿһ�ж���һ�������uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//�Զ���������η���
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//��ͼ�任
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//��ʾ1�����ݶ����ȡ���Ĺ���ǿ�Ⱥ�uv���в�ֵ��Ȼ��ͨ��uv�����ȡͼƬ����ɫ�������Թ���ǿ�Ⱦ���������ɫ
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		//��ʾ1��
		float intensity = varying_intensity * bar;
		Vec2f uv = varying_uv * bar;
		//ѡ���ȡ�ĸ�����ͼƬ
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) color[i] = color[i] * intensity;
		return false;
	}
};

//��ʾ2������ǿ�Ȳ���ʹ����������Ĺ���ǿ��ֵ���в�ֵ
struct TextureShader2 :public IShader
{
	//������ɫ��д�룬Ƭ����ɫ����ȡ��
	Vec3f varying_intensity;
	mat<2, 3, float> varying_uv;
	mat<4, 4, float> uniform_M;
	mat<4, 4, float> uniform_MIT;
	//��ʾ1��//�Զ���������ϵ���б任���Լ���ȡ�����������uv�͹���ǿ�ȣ�����ǿ�ȵ��ڷ��߳��Թ���������

	//��ʾ2������ǿ�Ȳ���ʹ����������Ĺ���ǿ��ֵ���в�ֵ
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//����uv,�����������֣�ÿһ�ж���һ�������uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//�Զ���������η���
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//��ͼ�任
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//���������һ��ģ�ͣ��䷨�����������Ҹ��������Ҹ�ģ���÷���ӳ����б任����������ʹ��ӳ����б任������ԭʼӳ�����ķ������ת��
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		Vec2f uv = varying_uv * bar;
		//�������ı任
		Vec3f n=proj<3>(uniform_MIT*embed<4>(model->normal(uv))).normalize();
		//��������������ϵ�任
		Vec3f L = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		//��� n �� l ����һ������ r = 2n<n��l> - l
		Vec3f r = (n * (n * L * 2.f) - L).normalize();
		float diffuse = std::max(0.f, n * L);
		//pow��������x��y�η�
		float specular = pow(std::max(r.z, 0.f), model->specular(uv));
		//ѡ���ȡ�ĸ�����ͼƬ
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + color[i] * (diffuse + 0.6 * specular), 255);
		return false;
	}
};

//Phong��ɫ
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
	//�洢uv����
	mat<2, 3, float> varying_uv;
	//�Ѷ��㰴�д���
	mat<4, 3, float> varying_tri;
	//�洢ԭʼ������
	mat<3, 3, float> varying_nrm;
	//�洢ȥ����η��̵Ķ�������
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
		//��ԭ��������ֵ
		Vec3f bn = (varying_nrm * bar).normalize();
		//��ԭuv��ֵ
		Vec2f uv = varying_uv * bar;

		mat<3, 3, float> A;
		//��һ�����������������㣬����A�ĵ�����
		//���д洢
		A[0] = ndc_tri.col(1) - ndc_tri.col(0);
		A[1] = ndc_tri.col(2) - ndc_tri.col(0);
		A[2] = bn;

		//�������
		mat<3, 3, float> AI = A.invert();
		Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0],varying_uv[0][2] - varying_uv[0][0],0.f);
		Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0],0.f);
		
		//���д洢
		mat<3, 3, float> B;
		B.set_col(0, i.normalize());
		B.set_col(1, j.normalize());
		B.set_col(2, bn);
		
		//���Ծ���B����������任
		Vec3f n = (B * model->normal(uv)).normalize();
		//���ܰѹ��շ��������㣬��Ϊ����ÿ�����ض��ѹ�����һ�Σ��ۼ�Ч���仯̫�죬ͼ�������������
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
	//Ϊʲô����3/4width��height
	viewport(width / 8, height / 8, width*3/4 , height*3/4);
	//norm����return std::sqrt(x * x + y * y + z * z)
	projection(-1.f / (eye - center).norm());
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	//gouraud��Ӱ��ɫ
	//GouraudShader shader;

	//������ɫ
	//TextureShader1 shader;
	/*TextureShader2 shader;
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT=(Projection*ModelView).invert_transpose();*/

	light_dir = proj<3>((Projection * ModelView) * embed<4>(light_dir)).normalize();

	Shader shader;
	//model->nfaces()���������ε�����
	for (int i = 0; i < model->nfaces(); i++) {
		//������������
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
