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
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        //存储纹理颜色
        if (!line.compare(0, 3, "vt ")) {
            //因为有"vt"两个字符，所以需要两次
            iss >> trash;
            iss >> trash;
            Vec3f tex;
            for (int i = 0; i < 3; i++) iss >> tex.raw[i];
            textures_.push_back(tex);
        }
        //扫描到三角形面行
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int  idx, itrash, lastrash;
            iss >> trash;
            //取face行中的单词中第一个数字
            while (iss >> idx >> trash >> itrash >> trash >> lastrash) {
                idx--; // in wavefront obj all indices start at 1, not zero
                f.push_back(idx);
                //载入纹理颜色
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
//获取顶点的数量
int Model::nverts() {
    return (int)verts_.size();
}
//获取三角形的数量
int Model::nfaces() {
    return (int)faces_.size();
}
//根据idx获取三角形的三个顶点坐标的idx
std::vector<int> Model::face(int idx) {
    return faces_[idx];
}
//获取顶点
Vec3f Model::vert(int i) {
    return verts_[i];
}
//获取纹理
Vec3f Model::texture(int i)
{
    return textures_[i] ;
}

