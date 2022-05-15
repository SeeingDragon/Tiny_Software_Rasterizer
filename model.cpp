#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"


Model::Model(const char* filename) : verts_(), faces_() {
    //ifstream读取文件
    std::ifstream in;
    //打开文件并读入,std::ifstream::in表示打开文件用于读取。
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    //eof中的方法是判断到达了文件结束符EOF，EOF是最后一个字符的下一个字符
    while (!in.eof()) {
        //逐行扫描in,把一行的数据传递给line
        std::getline(in, line);
        //iss获取到line的首个字母（单词）的指针，istringstream从string中读取数据,c_str()返回当前字符串的首字母地址,逐个单词扫描（空格符为间隔）
        std::istringstream iss(line.c_str());
        char trash;
        //compare函数从line的第0个字符开始扫描两个字符，与"v "(注意v有空格)进行比较，相同则返回0，不同则返回1
        if (!line.compare(0, 2, "v ")) {
            //iss把扫描到的 "v"单词存入trash,然后指针往下一个单词去。
            iss >> trash;
            Vec3f v;
            //存储一个顶点的x，y，z数据
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        //存储纹理颜色
        if (!line.compare(0, 3, "vt ")) {
            //因为有"vt"两个字符，所以需要两次
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uv_.push_back(uv);
        }
        //扫描到Gouraud阴影向量
        if (!line.compare(0, 3, "vn ")) {
            //因为有"vt"两个字符，所以需要两次
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            norms_.push_back(n.normalize());
        }
        //扫描到三角形面行
        else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            //取face行中的单词中第一个数字
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
    //进行垂直翻转，不然坐标是反的；
    diffusemap_.flip_vertically();
    normalmap_.flip_vertically();
    specualrmap_.flip_vertically();
}

Model::~Model() {
}
//获取顶点的数量
int Model::nverts() {
    return (int)verts_.size();
}
//获取三角形的数量
int Model::nfaces() {
    return (int)faces_.size();
}
//返回整个三角形面，得到三角形的三个点
std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i = 0; i < (int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
    return face;
}
//获取顶点
Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::vert(int iface, int nthvert)
{
    return verts_[faces_[iface][nthvert][0]];
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage &img)
{
    //定义纹理图片文件名
    std::string texfile(filename);
    //find_last_of函数寻找等于给定字符序列中字符之一的最后字符，找到最后一个".",没有找到返回string::nops;img.read_tga_file(texfile.c_str()
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


//获取纹理
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
    //把法线向量的颜色值重新转换为向量值，但是为什么rgb对应zyx
    //公式 c=(n+1)/2 *255
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
