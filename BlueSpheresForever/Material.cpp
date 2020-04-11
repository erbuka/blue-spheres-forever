#include "Material.h"
#include "ShaderProgram.h"

namespace bsf
{
	void PBRMaterial::Bind(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model)
	{
		auto& p = Program;

		p->Use();
		p->UniformMatrix4f("uProjection", projection);
		p->UniformMatrix4f("uView", view);
		p->UniformMatrix4f("uModel", model);

	}
}