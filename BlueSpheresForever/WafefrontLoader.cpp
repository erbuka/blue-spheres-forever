#include "BsfPch.h"

#include "WafefrontLoader.h"
#include "VertexArray.h"

#include <sstream>


namespace bsf
{

	const Ref<Model> WavefrontModel::CreateModel(GLenum usage)
	{
		auto result = MakeRef<Model>();

		for (const auto& mesh : Meshes)
		{
			std::vector<Vertex> data(mesh.Positions.size());

			for (uint32_t i = 0; i < mesh.Positions.size(); i++)
			{
				data[i].Position0 = mesh.Positions[i];
				data[i].Normal0 = mesh.Normals[i];
				data[i].UV = mesh.Uvs[i];

				data[i].Tangent0 = { 0.0f, 0.0f, 0.0f };
				data[i].Binormal0 = { 0.0f, 0.0f, 0.0f };
			}

			auto va = Ref<VertexArray>(new VertexArray({
				{ "aPosition", AttributeType::Float3 },
				{ "aNormal", AttributeType::Float3 },
				{ "aTangent", AttributeType::Float3 },
				{ "aBinormal", AttributeType::Float3 },
				{ "aUv", AttributeType::Float2 }
				}));

			va->SetData(data.data(), data.size(), usage);

			result->push_back(va);
		}

		return result;

	}

	Ref<WavefrontModel> WavefrontLoader::Load(const std::string& fileName)
	{
		std::ifstream stdIs;

		stdIs.open(fileName, std::ios_base::in);

		if (!stdIs.is_open())
		{
			BSF_ERROR("Could not open file: {0}", fileName);
			return nullptr;
		}

		BSF_INFO("Loading model from: {0}", fileName);

		auto parseVec3 = [&](const std::string& str) -> glm::vec3 {
			glm::vec3 v;
			std::stringstream(str) >> v.x >> v.y >> v.z;
			return v;
		};

		auto parseVec2 = [&](const std::string& str) -> glm::vec2 {
			glm::vec2 v;
			std::stringstream(str) >> v.x >> v.y;
			return v;
		};

		using Faces = std::vector<std::array<int32_t, 3>>;

		auto parseFace = [&](Faces& faces, const std::string& str) {
			std::stringstream ss(str);
			std::vector<std::string> vertices;

			while (ss.good())
			{
				std::string vertex;
				ss >> vertex;

				if (!vertex.empty())
					vertices.push_back(vertex);
			}

			if (vertices.size() != 3)
			{
				BSF_ERROR("Not triangle face found");
				return;
			}

			for (const auto& v : vertices)
			{
				std::array<int32_t, 3> indices = { -1, -1, -1 };

				auto pos0 = v.find('/');
				auto pos1 = v.find('/', pos0 + 1);

				if (pos1 == std::string::npos) // v/vt
				{
					indices[0] = std::atoi(v.substr(0, pos0).c_str());
					indices[1] = std::atoi(v.substr(pos0 + 1).c_str());
					indices[2] = -1;
				}
				else
				{
					if (pos0 == pos1 - 1) // v//vn
					{

						indices[0] = std::atoi(v.substr(0, pos0).c_str()) - 1;
						indices[1] = -1;
						indices[2] = std::atoi(v.substr(pos1 + 1).c_str()) - 1;
					}
					else // v/vt/vn
					{
						indices[0] = std::atoi(v.substr(0, pos0).c_str()) -1;
						indices[1] = std::atoi(v.substr(pos0 + 1, pos1 - pos0 - 1).c_str()) - 1;
						indices[2] = std::atoi(v.substr(pos1 + 1).c_str()) - 1;
					}
				}

				faces.push_back(indices);

			}

		};

		std::string currentGroup = "default";
		std::vector<glm::vec3> positions, normals;
		std::vector<glm::vec2> uvs;
		std::unordered_map<std::string, Faces> groups;

		// Parse the file
		while (stdIs.good())
		{
			std::string line;
			std::getline(stdIs, line);
	
			// TODO: trim line

			// Skip empty lines
			if (line.empty())
				continue;

			if (line[0] == '#') // Comment
			{
				BSF_INFO("{0}", line);
				continue;
			}

			switch (line[0])
			{
			case 'v':
				switch (line[1])
				{
				case 'n': normals.push_back(parseVec3(line.substr(2))); break;
				case 't': uvs.push_back(parseVec2(line.substr(2))); break;
				default: positions.push_back(parseVec3(line.substr(1))); break;
				}
				break;
			case 'o':
			case 'g':
				std::stringstream(line.substr(1)) >> currentGroup;
				break;
			case 'f': // faces
				parseFace(groups[currentGroup], line.substr(1));
				break;
			default:
				BSF_WARN("Unknown token: {0}", line[0]);
			}

		}

		stdIs.close();

		// Create the actual model object

		auto model = MakeRef<WavefrontModel>();

		for (const auto& group : groups)
		{
			WavefrontMesh mesh;

			mesh.Name = group.first;

			for (const auto& indices : group.second)
			{
				mesh.Positions.push_back(indices[0] == -1 ? glm::vec3{ 0.0f, 0.0f, 0.0f } : positions[indices[0]]);
				mesh.Uvs.push_back(indices[1] == -1 ? glm::vec2{ 0.0f, 0.0f } : uvs[indices[1]]);
				mesh.Normals.push_back(indices[2] == -1 ? glm::vec3{ 0.0f, 0.0f, 0.0f } : normals[indices[2]]);
			}

			model->Meshes.push_back(mesh);
		}


		return model;


	}
}