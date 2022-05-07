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
	std::vector<Vec3f> uv_;
	std::vector<Vec3f> tex_coord;
	//存储Gouraud阴影向量
	std::vector<Vec3f> norms_;
	//存储三角形面
	std::vector<std::vector<int> > faces_;

	void load_texture(std::string filename, const char* suffix, TGAImage& img);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec3f texture(int i);
	Vec3f Gour(int i);
	Vec2i uv(int iface, int nvert);
	TGAColor diffuse(Vec2i uv);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
