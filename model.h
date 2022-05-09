#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
	//�洢��������
	std::vector<Vec3f> verts_;
	//�洢��������
	std::vector<Vec2f> uv_;
	//�洢Gouraud��Ӱ����
	std::vector<Vec3f> norms_;
	//�洢��������
	std::vector<std::vector<Vec3i> > faces_;

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
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
