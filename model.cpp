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
        //�洢������ɫ
        if (!line.compare(0, 3, "vt ")) {
            //��Ϊ��"vt"�����ַ���������Ҫ����
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        }
        //ɨ�赽Gouraud��Ӱ����
        if (!line.compare(0, 3, "vn ")) {
            //��Ϊ��"vt"�����ַ���������Ҫ����
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            norms_.push_back(n.normalize());
        }
        //ɨ�赽����������
        else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            //ȡface���еĵ����е�һ������
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) tmp[i]--;
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() <<"vt# "<< uv_.size() <<"vn# "<< norms_.size() << " f# " << faces_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
    load_texture(filename, "_nm_tangent.tga", normalmap_);
    load_texture(filename, "_spec.tga", specualrmap_);
    //���д�ֱ��ת����Ȼ�����Ƿ��ģ�
    diffusemap_.flip_vertically();
    normalmap_.flip_vertically();
    specualrmap_.flip_vertically();
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
//���������������棬�õ������ε�������
std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i = 0; i < (int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
    return face;
}
//��ȡ����
Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::vert(int iface, int nthvert)
{
    return verts_[faces_[iface][nthvert][0]];
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage &img)
{
    //��������ͼƬ�ļ���
    std::string texfile(filename);
    //find_last_of����Ѱ�ҵ��ڸ����ַ��������ַ�֮һ������ַ����ҵ����һ��".",û���ҵ�����string::nops;img.read_tga_file(texfile.c_str()
    size_t dot = filename.find_last_of(".");
    if (dot != std::string::npos)
    {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file" << texfile << " loading" << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
    texfile = filename.substr(0, dot) + suffix;
    std::cerr << "texture file" << texfile << "loading" << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
}


//��ȡ����
Vec2f Model::uv(int iface,int nthvert)
{
    return uv_[faces_[iface][nthvert][1]] ;
}

Vec3f Model::normal(int iface,int nthvert)
{
    int idx = faces_[iface][nthvert][2];
    return norms_[idx].normalize();
}

TGAColor Model::diffuse(Vec2f uvf)
{
    Vec2i uv(int(uvf[0] * diffusemap_.get_width()), int(uvf[1] * diffusemap_.get_height()));
    return diffusemap_.get(uv.x, uv.y);
}

Vec3f Model::normal(Vec2f uvf)
{
    Vec2i uv(int(uvf[0] * normalmap_.get_width()), int(uvf[1] * normalmap_.get_height()));
    TGAColor c = normalmap_.get(uv.x, uv.y);
    //�ѷ�����������ɫֵ����ת��Ϊ����ֵ������Ϊʲôrgb��Ӧzyx
    //��ʽ c=(n+1)/2 *255
    Vec3f res;
    for (int i = 0; i < 3; i++)
    {
        res[2-i] = (float)c[i] / 255.f * 2.f - 1.f;
    }
    return res;
}

float Model::specular(Vec2f uvf)
{
    Vec2i uv(int(uvf[0] * specualrmap_.get_width()), int(uvf[1] * specualrmap_.get_height()));
    return specualrmap_.get(uv.x,uv.y)[0]/1.f;
}
