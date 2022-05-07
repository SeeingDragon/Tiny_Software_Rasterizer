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
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        //ɨ�赽Gouraud��Ӱ����
        if (!line.compare(0, 3, "vn ")) {
            //��Ϊ��"vt"�����ַ���������Ҫ����
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            norms_.push_back(n.normalize());
        }
        //�洢������ɫ
        if (!line.compare(0, 3, "vt ")) {
            //��Ϊ��"vt"�����ַ���������Ҫ����
            iss >> trash >> trash;
            Vec3f uv;
            for (int i = 0; i < 3; i++) iss >> uv[i];
            tex_coord.push_back(uv);
        }
        
        //ɨ�赽����������
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int  idx, itrash, vntrash;
            iss >> trash;
            //ȡface���еĵ����е�һ������
            while (iss >> idx >> trash >> itrash >> trash >> vntrash) {
                idx--; // in wavefront obj all indices start at 1, not zero
                f.push_back(idx);
                //����������ɫ
                itrash--;
                f.push_back(itrash);
                vntrash--;
                f.push_back(vntrash);
            }
            faces_.push_back(f);
            
        }
    }
    std::cerr << "# v# " << verts_.size() <<"vt# "<< uv_.size() <<"vn# "<< norms_.size() << " f# " << faces_.size() << std::endl;
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
    return tex_coord[i] ;
}

Vec3f Model::Gour(int i)
{
    return norms_[i];
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage& img)
{
    size_t dot = filename.find_last_of(".");
    if (dot == std::string::npos) return;
    std::string texfile = filename.substr(0, dot) + suffix;
    std::cerr << "texture file" << texfile << "loading" << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
}


