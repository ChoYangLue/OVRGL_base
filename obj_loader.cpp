

#include <fstream>
#include <vector>
#include "OVR_CAPI_GL.h"
#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"

#include "obj_loader.h"
#include "debug.h"


#define ISOK( _a ) { if(_a < 0) { printf("ISOK() Failed line: %d\n", __LINE__); return false;} }

using namespace OVR;

struct MtlModel
{
	std::string name;
	float ns;
	Vector3f ka;
	Vector3f kd;
	Vector3f ks;
	Vector3f ke;
	float ni;
	int illum;
	std::string tex;
	int vertex_ind[2];
};

struct ObjModel
{
	std::string dir_path;
	std::vector<MtlModel> materials;

	int GetMaterialIndexByName(const std::string& _name)
	{
		int ret_ind = 0;
		for (auto& mt:materials) {
			if (mt.name == _name) return ret_ind;
			ret_ind++;
		}
		return -1;
	}
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

bool loadMTL(std::vector<MtlModel>& materials, const std::string& _filePath)
{
	if (_filePath.empty()) {
		LOG("file path is empty \n");
		return false;
	}

	// ファイルオープン
	std::ifstream mtl_ifs(_filePath);
	if (!mtl_ifs) {
		mtl_ifs.close();
		LOG("mylFile is Open Failed \n");
		return false;
	}


	int mtl_index = -1;
	while (true) {
		std::string buf;
		std::getline(mtl_ifs, buf);

		if (!mtl_ifs) {
			DBGLOG("objFile is Read end");
			break;
		}

		if (buf.find("newmtl ") != std::string::npos) {
			
			std::vector<std::string> mtl_name = splitWithSpace(buf);

			MtlModel mat_temp;
			mat_temp.name = mtl_name[0];
			materials.push_back(mat_temp);
			mtl_index++;
		}
		else if (buf.find("map_Kd ") != std::string::npos) {
			// テクスチャ名
			ISOK(mtl_index);

			std::vector<std::string> tex_name = splitWithSpace(buf);
			//materials[mtl_index].tex = tex_name[0];
		}
		else if (buf.find("Kd ") != std::string::npos) {
			// ディフューズカラー	RGB(0.0～1.0)
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].kd.x = std::stof(spl_list[0]);
			materials[mtl_index].kd.y = std::stof(spl_list[1]);
			materials[mtl_index].kd.z = std::stof(spl_list[2]);

		}
		else if (buf.find("Ka ") != std::string::npos) {
			// アンビエントカラー	RGB(0.0～1.0)
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].ka.x = std::stof(spl_list[0]);
			materials[mtl_index].ka.y = std::stof(spl_list[1]);
			materials[mtl_index].ka.z = std::stof(spl_list[2]);
		}
		else if (buf.find("Ks ") != std::string::npos) {
			// スペキュラーカラー	RGB(0.0～1.0)
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].ks.x = std::stof(spl_list[0]);
			materials[mtl_index].ks.y = std::stof(spl_list[1]);
			materials[mtl_index].ks.z = std::stof(spl_list[2]);
		}
		else if (buf.find("Ke ") != std::string::npos) {
			// スペキュラーカラー	RGB(0.0～1.0)
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].ke.x = std::stof(spl_list[0]);
			materials[mtl_index].ke.y = std::stof(spl_list[1]);
			materials[mtl_index].ke.z = std::stof(spl_list[2]);
		}
		else if (buf.find("Ns ") != std::string::npos) {
			if (mtl_index < 0) {
				LOG("mtl index is -1 \n");
				mtl_ifs.close();
				return false;
			}

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].ns = std::stof(spl_list[0]);
		}
		else if (buf.find("Ni ") != std::string::npos) {
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].ni = std::stof(spl_list[0]);
		}
		else if (buf.find("illum ") != std::string::npos) {
			ISOK(mtl_index);

			std::vector<std::string> spl_list = splitWithSpace(buf);
			materials[mtl_index].illum = std::stoi(spl_list[0]);
		}

	}
	mtl_ifs.close();

	return true;
}

bool loadOBJ(Model& data, const std::string& _filePath)
{
	
	if (_filePath.empty()) {
		LOG("file path is empty \n");
		return false;
	}

	ObjModel obj_model;

	// モデルファイルのパスからモデルフォルダのパスを抽出
	obj_model.dir_path = splitDirPath(_filePath);
	DBGLOG("dir: %s", obj_model.dir_path.c_str() );

	// ファイルオープン
	std::ifstream obj_ifs(_filePath);
	if (!obj_ifs) {
		obj_ifs.close();
		LOG("objFile is Open Failed \n");
		return false;
	}

	int vertex_index = 0;
	int pre_vertex_index = 0;
	std::vector<MtlModel> materials;
	while (true) {
		std::string buf;
		std::getline(obj_ifs, buf);

		if (!obj_ifs) {
			DBGLOG("objFile is Read end");
			break;
		}

		if (buf.find("mtllib ") != std::string::npos) {
			// material file
			std::vector<std::string> mtllib_name = splitWithSpace(buf);
			DBGLOG("read mtl file: %s", mtllib_name[0].c_str() );

			bool ret = loadMTL(obj_model.materials, obj_model.dir_path+"/"+mtllib_name[0]);
			if (!ret) {
				LOG("mtlFile is read Failed \n");
				return false;
			}
		}
		else if (buf.find("v ") != std::string::npos) {
			// 頂点データ
			//DBGLOG("vertex: %s \n", buf.c_str());

			Vector3f  Pos;
			std::vector<std::string> vec_str_list = splitWithSpace(buf);
			Pos.x = std::stof(vec_str_list[1]);
			Pos.y = std::stof(vec_str_list[0]);
			Pos.z = std::stof(vec_str_list[2]);
			
			Vertex vvv;
			vvv.Pos = Pos;
			vvv.U = 1;
			vvv.V = 1;

			// Make vertices, with some token lighting
			DWORD c = 0xff4040ff;
			float dist1 = (vvv.Pos - Vector3f(-2, 4, -2)).Length();
			float dist2 = (vvv.Pos - Vector3f(3, 4, -3)).Length();
			float dist3 = (vvv.Pos - Vector3f(-4, 3, 25)).Length();
			int   bri = rand() % 160;

			float B = ((c >> 16) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float G = ((c >> 8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float R = ((c >> 0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			vvv.C = (c & 0xff000000) +
				((R > 255 ? 255 : DWORD(R)) << 16) +
				((G > 255 ? 255 : DWORD(G)) << 8) +
				(B > 255 ? 255 : DWORD(B));
			data.AddVertex(vvv);
			vertex_index++;
		}
		else if (buf.find("vt ") != std::string::npos) {
			// １つの頂点のテクスチャ座標値
			std::vector<std::string> uv_str_list = splitWithSpace(buf);

		}
		else if (buf.find("vn ") != std::string::npos) {
			// １つの頂点の法線
			std::vector<std::string> norm_str_list = splitWithSpace(buf);

		}
		else if (buf.find("usemtl ") != std::string::npos) {
			// １つの頂点の法線
			std::vector<std::string> norm_str_list = splitWithSpace(buf);
			DBGLOG("mtl name: %s", norm_str_list[0].c_str() );

			int ind_t = obj_model.GetMaterialIndexByName(norm_str_list[0]);
			ISOK(ind_t);
			obj_model.materials[ind_t].vertex_ind[0] = pre_vertex_index;
			obj_model.materials[ind_t].vertex_ind[1] = vertex_index;

			pre_vertex_index = vertex_index;
		}
		else if (buf.find("f ") != std::string::npos) {
			// 頂点座標値番号/テクスチャ座標値番号/頂点法線ベクトル番号

			std::vector<std::string> f_str_list = splitWithSpace(buf);
			if (f_str_list.size() > 3) {
				LOG("objFile is not triangle face \n");
				return false;
			}

			for (auto& li : f_str_list) {
				std::vector<std::string> face_str_list = splitWithSlash(li);

				// 1から始まりなので0から始まりにする
				data.AddIndex((GLushort)std::stoi(face_str_list[0])-1 );
			}
			
		}

	}
	obj_ifs.close();


	for (auto& mt:obj_model.materials) {
		for (int i = mt.vertex_ind[0]; i < mt.vertex_ind[1]; i++) {
			data.Vertices[i].C = GetColor(255, 0, 0);
		}
	}

	return true;
	
}
