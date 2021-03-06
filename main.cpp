#include <vector> 
#include <iostream> 
#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"


Model* model = NULL;
float* shadowbuffer = NULL;
const int width = 800;
const int height = 800;

Vec3f eye(1.2, -.8, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

//GRAYSCALE：0是黑色，254是白色  
TGAImage total(1024, 1024, TGAImage::GRAYSCALE);
TGAImage occl(1024, 1024, TGAImage::GRAYSCALE);

struct ZShader : public IShader
{
	mat<4, 3, float> varying_tri;
	virtual Vec4f vertex(int iface, int nthvert)
	{
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Projection * ModelView * gl_vertex;
		varying_tri.set_col(nthvert, gl_vertex);
		return gl_vertex;
	}
	virtual bool fragment(Vec3f gl_fragcoord,Vec3f bar,TGAColor& color)
	{
		//color = TGAColor(255, 255, 255) * ((gl_fragcoord.z)/ 2000.f);
		color = TGAColor(0, 0, 0);
		return false;
	}
};

struct uvShader : public IShader
{
	mat<2, 3, float> varying_uv;
	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Viewport * Projection * ModelView * gl_vertex;
		return gl_vertex;
	}

	virtual bool fragment(Vec3f gl_fragcoords, Vec3f bar, TGAColor& color)
	{//对uv坐标进行插值
		Vec2f uv = varying_uv * bar;
		int idx = gl_fragcoords.x + gl_fragcoords.y * width;
		//在这个方向的光下该像素可见。
		if (std::abs(shadowbuffer[idx] - gl_fragcoords.z) < 1e-2)
		{
			occl.set(uv.x * 1024, uv.y * 1024, TGAColor(254));
		}
		color = TGAColor(255, 0, 0);
		return false;
	}
};

//该着色器读取自己生成的环境光贴图
struct AOShader : public IShader
{
	mat<2, 3, float> varying_uv;
	mat<4, 3, float> varying_tri;
	virtual Vec4f vertex(int iface,int nthvert)
	{
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Projection * ModelView * gl_vertex;
		varying_tri.set_col(nthvert,gl_vertex);
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		return gl_vertex;
	}

	virtual bool fragment(Vec3f gl_fragcoords, Vec3f bar,TGAColor& color)
	{
		Vec2f uv = varying_uv * bar;
		color = model->occlusion(uv);
		return false;
	}
};



//球面均匀取点
Vec3f rand_point_on_unit_sphere()
{
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	float theta = 2.f * M_PI * u;
	float phi = acos(2.f * v - 1.f);
	return Vec3f(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

float max_elevation_angle(float* zbuffer, Vec2f p, Vec2f dir)
{
	float maxangle = 0;
	for (float t = 0; t < 1000.f; t += 1.)
	{
		Vec2f cur = p + dir * t;
		if (cur.x >= width || cur.y >= height || cur.x < 0 || cur.y < 0) return maxangle;

		float distance = (p - cur).norm();
		if (distance < 1.f) continue;
		float elevation = zbuffer[int(cur.x) + int(cur.y) * width] - zbuffer[int(p.x) + int(p.y) * width];
		maxangle = std::max(maxangle, atanf(elevation / distance));
		
	}
	return maxangle;
}


//argc 是 argument count的缩写，表⽰传⼊main函数的参数个数；
//argv 是 argument vector的缩写，表⽰传⼊main函数的参数序列或指针，并且第⼀个参数argv[0]⼀定是程序的名称，并且包含了程序所在的完整路径，所以确切的说需要我们输⼊的main函数的参数个数应该是argc - 1个；
int main(int argc, char** argv)
{
	
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		//diablo3_pose  //african_head
		model = new Model("F:\\GraphicsLearn\\Tiny_Software_Rasterizer\\obj\\diablo3_pose\\diablo3_pose.obj");
	}
	float* zbuffer = new float[width * height];
	shadowbuffer = new float[width * height];
	for (int i = 0; i < width * height; i++) zbuffer[i]= shadowbuffer[i] = -std::numeric_limits<float>::max();

	TGAImage frame(width, height, TGAImage::GRAYSCALE);
	lookat(eye, center, up);
	//为什么采用3/4width和height
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	//norm函数return std::sqrt(x * x + y * y + z * z)
	projection(-1.f / (eye - center).norm());

	/*AOShader shader;
	for (int i = 0; i < model->nfaces(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			shader.vertex(i, j);
		}
		triangle(shader.varying_tri, shader, frame, zbuffer);
	}
	frame.flip_vertically();
	frame.write_tga_file("output.tga");*/

	//获得zbuffer
	ZShader zshader;
	for (int i = 0; i < model->nfaces(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			zshader.vertex(i, j);
		}
		triangle(zshader.varying_tri, zshader, frame, zbuffer);
	}
	

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//如果该zbuffer[idx]值无限小，则该像素点不被三角形所覆盖，所以直接跳出该循环
			//1e5 1*10的五次方
			if (zbuffer[x + y * width] < -1e5) continue;
			float total = 0;
			//1e-4 1*10的负4次方
			//循环八次，减掉1e-4或许是为了误差，保证循环八次，不是七次也不是九次
			for (float a = 0; a < M_PI * 2 - 1e-4; a += M_PI / 4)
			{
				//计算z缓冲区每个点的立体角，但我们将其近似为（90°-max_elevation_angle）/ 8的总和
				total += M_PI / 2 - max_elevation_angle(zbuffer, Vec2f(x, y), Vec2f(cos(a), sin(a)));
			}
			total /= (M_PI / 2) * 8;
			total = pow(total, 100.f);
			frame.set(x, y, TGAColor(total * 255, total * 255, total * 255));
			
		}
		std::cout << x << std::endl;
	}
	frame.flip_vertically();
	frame.write_tga_file("framebuffer.tga");

	delete model;
	delete[] zbuffer;
	delete[] shadowbuffer;
	return 0;
}


//以下代码均匀在球面上生成一千个向量模拟光照，生成环境光贴图。此时眼睛和光照所处位置相同
	//{
	//	const int nrenders = 1;
	//	for (int iter = 1; iter <= nrenders; iter++)
	//	{
	//		std::cerr << iter << " from" << nrenders << std::endl;
	//		for (int i = 0; i < 3; i++) up[i] = (float)rand() / (float)RAND_MAX;
	//		eye = rand_point_on_unit_sphere();
	//		eye.y = std::abs(eye.y);
	//		std::cout << "v " << eye << std::endl;

	//		for (int i = width * height; i--; zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max());

	//		TGAImage frame(width, height, TGAImage::RGB);

	//		lookat(eye, center, up);
	//		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	//		projection(0);

	//		//第一次渲染，将深度值存储在shadowbuffer上。
	//		ZShader zshader;

	//		for (int i = 0; i < model->nfaces(); i++)
	//		{
	//			for (int j = 0; j < 3; j++)
	//			{
	//				screen_coords[j] = zshader.vertex(i, j);
	//			}
	//			triangle(screen_coords, zshader, frame, shadowbuffer);
	//		}
	//		frame.flip_vertically();
	//		frame.write_tga_file("framebuffer.tga");

	//		uvShader shader;
	//		occl.clear();
	//		for (int i = 0; i < model->nfaces(); i++)
	//		{
	//			for (int j = 0; j < 3; j++)
	//			{
	//				screen_coords[j] = shader.vertex(i, j);
	//			}
	//			triangle(screen_coords, shader, frame, zbuffer);
	//		}

	//		for (int i = 0; i < 1024; i++)
	//		{
	//			for (int j = 0; j < 1024; j++)
	//			{
	//				float tmp = total.get(i, j)[0];
	//				total.set(i, j, TGAColor((tmp * (iter - 1) + occl.get(i, j)[0]) / (float)iter + 0.5f));
	//			}
	//		}
	//	}
	//	total.flip_vertically();
	//	total.write_tga_file("occlusion.tga");
	//	occl.flip_vertically();
	//	occl.write_tga_file("occl.tga");
	//}