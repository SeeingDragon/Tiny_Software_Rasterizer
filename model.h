#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
	//存储顶点数据
	std::vector<Vec3f> verts_;
	//存储纹理坐标
	std::vector<Vec2f> uv_;
	//存储Gouraud阴影向量
	std::vector<Vec3f> norms_;
	//存储三角形面
	std::vector<std::vector<Vec3i> > faces_;

	//漫反射颜色贴图
	TGAImage diffusemap_;
	//法线向量贴图
	TGAImage normalmap_;
	//高光贴图
	TGAImage specualrmap_;
	//环境光贴图
	TGAImage occlusionmap_;
	void load_texture(std::string filename, const char* suffix, TGAImage& img);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec3f vert(int iface, int nthvert);
	Vec2f uv(int iface, int nthvert);
	Vec3f normal(int iface, int nthvert);
	TGAColor diffuse(Vec2f uvf);
	TGAColor occlusion(Vec2f uvf);
	Vec3f normal(Vec2f uvf);
	float specular(Vec2f uvf);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
