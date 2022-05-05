#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	//�洢��������
	std::vector<Vec3f> verts_;
	//�洢������ɫ
	std::vector<Vec3f> textures_;
	//�洢��������
	std::vector<std::vector<int> > faces_;
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec3f texture(int i);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
