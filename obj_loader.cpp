

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



// �ʂ��\�����钸�_���
struct Face {
	int vi[3];           // ���_�ԍ�
	int ti[3];           // �e�N�X�`�����W
	int ni[3];           // �@���x�N�g���ԍ�
};

// �}�e���A��
// TIPS �����o�ϐ��ɂ��炩���ߏ����l��^���Ă���
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
			// �Ō�̃p�X�̓t�@�C���Ȃ̂œǂ܂Ȃ�
			break;
		}

		ret_val += buf.substr(offset, pos - offset)+"/";

		offset = pos + 1;
	}
	return ret_val;
}

// �}�e���A����ǂݍ���
bool loadMaterial(std::map<std::string, Material>& material, const std::string& path) {

	std::ifstream stream(path);
	// �t�@�C�����J���Ȃ���΁A��̏���Ԃ�
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
			// �X�y�L�����w��
			ss >> material[cur_name].shininess;
		}
		else if (key == "Tr") {
			// ����
		}
		else if (key == "Kd") {
			// �g�U��
			ss >> material[cur_name].diffuse[0]
				>> material[cur_name].diffuse[1]
				>> material[cur_name].diffuse[2];
		}
		else if (key == "Ks") {
			// �X�y�L����
			ss >> material[cur_name].specular[0]
				>> material[cur_name].specular[1]
				>> material[cur_name].specular[2];
		}
		else if (key == "Ka") {
			// ����
			ss >> material[cur_name].ambient[0]
				>> material[cur_name].ambient[1]
				>> material[cur_name].ambient[2];
		}
		else if (key == "map_Kd") {
			// �e�N�X�`��
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

	// ���f���t�@�C���̃p�X���烂�f���t�H���_�̃p�X�𒊏o
	std::string dir_path = splitDirPath(_filePath);

	// �t�@�C���I�[�v��
	std::ifstream obj_ifs(_filePath);
	if (!obj_ifs) {
		obj_ifs.close();
		LOG("objFile is Open Failed \n");
		return false;
	}


	// �t�@�C������ǂݎ�����l���ꎞ�I�ɕێ����Ă����ϐ�
	std::vector<Vector3f> normal;
	std::vector<Vector2f> texture;
	std::string cur_mat("");       // �}�e���A����
	std::map<std::string, Material> material;
	std::map<std::string, std::vector<Face>> face;

	// NOTICE OBJ�`���̓f�[�^�̕��тɌ��܂肪�Ȃ��̂ŁA
	//        ��������S�Ă̏����t�@�C������ǂݎ��
	while (!obj_ifs.eof()) {
		std::string s;
		std::getline(obj_ifs, s);

		// TIPS:������X�g���[�����g��
		//      �����񁨐��l�̃v���O�������ׂ������Ă���
		std::stringstream ss(s);

		std::string key;
		ss >> key;
		if (key == "mtllib") {
			// �}�e���A��
			std::string m_path;
			ss >> m_path;
			bool ret = loadMaterial(material, dir_path+"/"+m_path);
			DBGLOG("mtl file ret: %d", ret);
		}
		else if (key == "usemtl") {
			// �K�p����}�e���A����ύX
			ss >> cur_mat;
		}
		else if (key == "v") {
			// ���_���W
			Vector3f vv;
			ss >> vv.y >> vv.x >> vv.z;
			Vertex vvv;
			vvv.Pos = vv;
			vvv.U = -1; vvv.V = -1;
			data.AddVertex(vvv);
		}
		else if (key == "vt") {
			// ���_�e�N�X�`�����W
			Vector2f uv;
			ss >> uv.x >> uv.y;
			texture.push_back(uv);
		}
		else if (key == "vn") {
			// ���_�@���x�N�g��
			Vector3f vv;
			ss >> vv.x >> vv.y >> vv.z;
			normal.push_back(vv);
		}
		else if (key == "f") {
			// �ʂ��\�����钸�_���
			// FIXME �R�p�`�݂̂̑Ή�
			Face f = {
				{ -1, -1, -1 },
				{ -1, -1, -1 },
				{ -1, -1, -1 }
			};

			for (int i = 0; i < 3; ++i) {
				std::string text;
				ss >> text;
				// TIPS '/' �ŋ�؂�ꂽ�����񂩂琔�l�ɕϊ����邽��
				//      ��Ustringstream�֕ϊ�
				std::stringstream fs(text);

				// f ���_���W�l�ԍ�/�e�N�X�`�����W�l�ԍ�/���_�@���x�N�g���ԍ�
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

	// material����
	for (auto& inm: material) {

		// face���� 
		for (auto& fas : face[inm.first]) {
			for (int i = 0; i < 3; i++) {
				data.AddIndex((GLushort)fas.vi[i]);
				//data.Vertices[fas.vi[i]].C = GetColor(255,255,255);
				//DBGLOG("col r: %i", (int)(inm.second.diffuse[0] * 255));
				if (data.Vertices[fas.vi[i]].U + data.Vertices[fas.vi[i]].V >= 0) {
					// ���Ƀe�N�X�`�����W���o�^����Ă���
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
