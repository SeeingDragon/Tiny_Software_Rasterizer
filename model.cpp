#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), faces_() {
    //ifstream��ȡ�ļ�
    std::ifstream in;
    //���ļ�������,std::ifstream::in��ʾ���ļ����ڶ�ȡ��
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    //eof�еķ������жϵ������ļ�������EOF��EOF�����һ���ַ�����һ���ַ�
    while (!in.eof()) {
        //����ɨ��in,��һ�е����ݴ��ݸ�line
        std::getline(in, line);
        //iss��ȡ��line���׸���ĸ�����ʣ���ָ�룬istringstream��string�ж�ȡ����,c_str()���ص�ǰ�ַ���������ĸ��ַ,�������ɨ�裨�ո��Ϊ�����
        std::istringstream iss(line.c_str());
        char trash;
        //compare������line�ĵ�0���ַ���ʼɨ�������ַ�����"v "(ע��v�пո�)���бȽϣ���ͬ�򷵻�0����ͬ�򷵻�1
        if (!line.compare(0, 2, "v ")) {
            //iss��ɨ�赽�� "v"���ʴ���trash,Ȼ��ָ������һ������ȥ��
            iss >> trash;
            Vec3f v;
            //�洢һ�������x��y��z����
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        //�洢������ɫ
        if (!line.compare(0, 3, "vt ")) {
            //��Ϊ��"vt"�����ַ���������Ҫ����
            iss >> trash;
            iss >> trash;
            Vec3f tex;
            for (int i = 0; i < 3; i++) iss >> tex.raw[i];
            textures_.push_back(tex);
        }
        //ɨ�赽����������
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int  idx, itrash, lastrash;
            iss >> trash;
            //ȡface���еĵ����е�һ������
            while (iss >> idx >> trash >> itrash >> trash >> lastrash) {
                idx--; // in wavefront obj all indices start at 1, not zero
                f.push_back(idx);
                //����������ɫ
                itrash--;
                f.push_back(itrash);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
}

Model::~Model() {
}
//��ȡ���������
int Model::nverts() {
    return (int)verts_.size();
}
//��ȡ�����ε�����
int Model::nfaces() {
    return (int)faces_.size();
}
//����idx��ȡ�����ε��������������idx
std::vector<int> Model::face(int idx) {
    return faces_[idx];
}
//��ȡ����
Vec3f Model::vert(int i) {
    return verts_[i];
}
//��ȡ����
Vec3f Model::texture(int i)
{
    return textures_[i] ;
}

