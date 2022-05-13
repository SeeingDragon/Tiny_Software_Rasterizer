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
	mat<2, 3, float> varying_uv;
	mat<4, 4, float> uniform_M;
	mat<4, 4, float> uniform_MIT;
	//��ʾ1��//�Զ���������ϵ���б任���Լ���ȡ�����������uv�͹���ǿ�ȣ�����ǿ�ȵ��ڷ��߳��Թ���������

	//��ʾ2������ǿ�Ȳ���ʹ����������Ĺ���ǿ��ֵ���в�ֵ
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//��ʾ1��
		//���ݷ��������������ǿ��
		//varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
		//����uv,�����������֣�ÿһ�ж���һ�������uv
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		//�Զ���������η���
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		//��ͼ�任
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
		return gl_Vertex;
	}

	//��ʾ1�����ݶ����ȡ���Ĺ���ǿ�Ⱥ�uv���в�ֵ��Ȼ��ͨ��uv�����ȡͼƬ����ɫ�������Թ���ǿ�Ⱦ���������ɫ

	//���������һ��ģ�ͣ��䷨�����������Ҹ��������Ҹ�ģ���÷���ӳ����б任����������ʹ��ӳ����б任������ԭʼӳ�����ķ������ת��
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		//��ʾ1��
		//float intensity = varying_intensity * bar;
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
	light_dir.normalize();
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	GouraudShader shader;
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT=(Projection*ModelView).invert_transpose();
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
