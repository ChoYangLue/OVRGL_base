

#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include "OVR_CAPI_GL.h"
#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"

#include "obj_loader.h"
#include "debug.h"


#define ISOK( _a ) { if(_a < 0) { printf("ISOK() Failed line: %d\n", __LINE__); return false;} }

using namespace OVR;



// 面を構成する頂点情報
struct Face {
	int vi[3];           // 頂点番号
	int ti[3];           // テクスチャ座標
	int ni[3];           // 法線ベクトル番号
};

// マテリアル
// TIPS メンバ変数にあらかじめ初期値を与えている
struct Material {
	GLfloat ambient[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat diffuse[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat specular[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat shininess = 80.0f;
	std::string texture_name = "";
	std::string name = "";
};


std::vector<std::string> splitWithSpace(const std::string& buf)
{
	auto offset = std::string::size_type(0);
	std::vector<std::string> ret_val;
	bool first_flag = true;
	while (1) {
		auto pos = buf.find(" ", offset);
		if (pos == std::string::npos) {
			//float a = std::stof(buf.substr(offset).c_str());
			//DBGLOG("vertex: %f", a);
			ret_val.push_back(buf.substr(offset));
			break;
		}

		if (first_flag == false) {
			//float ab = std::stof(buf.substr(offset, pos - offset).c_str());
			//DBGLOG("vertex: %f ", ab);
			//std::stof(buf.substr(offset, pos - offset).c_str());
			ret_val.push_back(buf.substr(offset, pos - offset));
		}

		offset = pos + 1;
		first_flag = false;
	}
	return ret_val;
}

std::vector<std::string> splitWithSlash(const std::string& buf)
{
	std::vector<std::string> ret_val;
	auto offset = std::string::size_type(0);
	while (1) {
		auto pos = buf.find("/", offset);
		if (pos == std::string::npos) {
			//float a = std::stof(buf.substr(offset).c_str());
			//DBGLOG("vertex: %f", a);
			ret_val.push_back(buf.substr(offset));
			break;
		}


		ret_val.push_back(buf.substr(offset, pos - offset));

		offset = pos + 1;
	}
	return ret_val;
}

std::string splitDirPath(const std::string& buf)
{
	std::string ret_val;
	auto offset = std::string::size_type(0);
	while (1) {
		auto pos = buf.find("/", offset);
		if (pos == std::string::npos) {
			// 最後のパスはファイルなので読まない
			break;
		}

		ret_val += buf.substr(offset, pos - offset)+"/";

		offset = pos + 1;
	}
	return ret_val;
}

// マテリアルを読み込む
bool loadMaterial(std::map<std::string, Material>& material, const std::string& path) {

	std::ifstream stream(path);
	// ファイルを開けなければ、空の情報を返す
	if (!stream) return false;

	std::string cur_name;

	while (!stream.eof()) {
		std::string s;
		std::getline(stream, s);

		std::stringstream ss(s);
		std::string key;
		ss >> key;
		if (key == "newmtl") {
			ss >> cur_name;
		}
		else if (key == "Ns") {
			// スペキュラ指数
			ss >> material[cur_name].shininess;
		}
		else if (key == "Tr") {
			// 透過
		}
		else if (key == "Kd") {
			// 拡散光
			ss >> material[cur_name].diffuse[0]
				>> material[cur_name].diffuse[1]
				>> material[cur_name].diffuse[2];
		}
		else if (key == "Ks") {
			// スペキュラ
			ss >> material[cur_name].specular[0]
				>> material[cur_name].specular[1]
				>> material[cur_name].specular[2];
		}
		else if (key == "Ka") {
			// 環境光
			ss >> material[cur_name].ambient[0]
				>> material[cur_name].ambient[1]
				>> material[cur_name].ambient[2];
		}
		else if (key == "map_Kd") {
			// テクスチャ
			ss >> material[cur_name].texture_name;

		}
	}

	return true;
}

bool loadOBJ(Model& data, const std::string& _filePath)
{
	
	if (_filePath.empty()) {
		LOG("file path is empty \n");
		return false;
	}

	// モデルファイルのパスからモデルフォルダのパスを抽出
	std::string dir_path = splitDirPath(_filePath);

	// ファイルオープン
	std::ifstream obj_ifs(_filePath);
	if (!obj_ifs) {
		obj_ifs.close();
		LOG("objFile is Open Failed \n");
		return false;
	}


	// ファイルから読み取った値を一時的に保持しておく変数
	std::vector<Vector3f> normal;
	std::vector<Vector2f> texture;
	std::string cur_mat("");       // マテリアル名
	std::map<std::string, Material> material;
	std::map<std::string, std::vector<Face>> face;

	// NOTICE OBJ形式はデータの並びに決まりがないので、
	//        いったん全ての情報をファイルから読み取る
	while (!obj_ifs.eof()) {
		std::string s;
		std::getline(obj_ifs, s);

		// TIPS:文字列ストリームを使い
		//      文字列→数値のプログラム負荷を下げている
		std::stringstream ss(s);

		std::string key;
		ss >> key;
		if (key == "mtllib") {
			// マテリアル
			std::string m_path;
			ss >> m_path;
			bool ret = loadMaterial(material, dir_path+"/"+m_path);
			DBGLOG("mtl file ret: %d", ret);
		}
		else if (key == "usemtl") {
			// 適用するマテリアルを変更
			ss >> cur_mat;
		}
		else if (key == "v") {
			// 頂点座標
			Vector3f vv;
			ss >> vv.y >> vv.x >> vv.z;
			Vertex vvv;
			vvv.Pos = vv;
			vvv.U = -1; vvv.V = -1;
			data.AddVertex(vvv);
		}
		else if (key == "vt") {
			// 頂点テクスチャ座標
			Vector2f uv;
			ss >> uv.x >> uv.y;
			texture.push_back(uv);
		}
		else if (key == "vn") {
			// 頂点法線ベクトル
			Vector3f vv;
			ss >> vv.x >> vv.y >> vv.z;
			normal.push_back(vv);
		}
		else if (key == "f") {
			// 面を構成する頂点情報
			// FIXME ３角形のみの対応
			Face f = {
				{ -1, -1, -1 },
				{ -1, -1, -1 },
				{ -1, -1, -1 }
			};

			for (int i = 0; i < 3; ++i) {
				std::string text;
				ss >> text;
				// TIPS '/' で区切られた文字列から数値に変換するため
				//      一旦stringstreamへ変換
				std::stringstream fs(text);

				// f 頂点座標値番号/テクスチャ座標値番号/頂点法線ベクトル番号
				{
					std::string v;
					std::getline(fs, v, '/');
					f.vi[i] = std::stoi(v) - 1;
				}
				{
					std::string v;
					std::getline(fs, v, '/');
					if (!v.empty()) {
						f.ti[i] = std::stoi(v) - 1;
					}
				}
				{
					std::string v;
					std::getline(fs, v, '/');
					if (!v.empty()) {
						f.ni[i] = std::stoi(v) - 1;
					}
				}
			}
			face[cur_mat].push_back(f);
		}
	}
	obj_ifs.close();


	DBGLOG("mat size: %zi", material.size() );

	// materialごと
	for (auto& inm: material) {

		// faceごと 
		for (auto& fas : face[inm.first]) {
			for (int i = 0; i < 3; i++) {
				data.AddIndex((GLushort)fas.vi[i]);
				//data.Vertices[fas.vi[i]].C = GetColor(255,255,255);
				//DBGLOG("col r: %i", (int)(inm.second.diffuse[0] * 255));
				if (data.Vertices[fas.vi[i]].U + data.Vertices[fas.vi[i]].V >= 0) {
					// 既にテクスチャ座標が登録されている
					Vertex vvv;
					vvv.Pos = data.Vertices[fas.vi[i]].Pos;
					vvv.C = GetColor((int)(inm.second.diffuse[0] * 255), (int)(inm.second.diffuse[1] * 255), (int)(inm.second.diffuse[2] * 255));
					vvv.U = texture[fas.ti[i]].x;
					vvv.V = -1.0f*texture[fas.ti[i]].y;
					data.AddVertex(vvv);
					fas.vi[i] = data.Vertices.size()-1;
				}
				else {
					data.Vertices[fas.vi[i]].C = GetColor((int)(inm.second.diffuse[0] * 255), (int)(inm.second.diffuse[1] * 255), (int)(inm.second.diffuse[2] * 255));
					data.Vertices[fas.vi[i]].U = texture[fas.ti[i]].x;
					data.Vertices[fas.vi[i]].V = -1.0f*texture[fas.ti[i]].y;
					DBGLOG("tex index: %f", texture[fas.ti[i]].x);
				}
				
			}
			
		}
		
	}

	return true;
	
}
