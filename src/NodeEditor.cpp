#include "NodeEditor.h"
#include "tinyfiledialogs.h"
#include "core/Engine3D.h"

namespace narvalengine {

	NodeEditor::~NodeEditor() {
		for (auto node : nodes) {
			delete node;
		}
		nodes.clear();
	}

	void NodeEditor::loadFromMaterial(Material* material, MaterialHandler *mh) {
		if (material->hasBSDF()){
			ImMaterialOutputNode* output = new ImMaterialOutputNode(++nodeIdStack, ++attIdStack, ++attIdStack);
			output->position = positionOffset;
			positionOffset += ImVec2(60, 60);
			nodes.push_back(output);

			BSDF* bsdf = material->bsdf;
			ImStandartSurfaceNode* surfaceNode = new ImStandartSurfaceNode(++nodeIdStack, ++attIdStack,
				++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack,
				&linkManager);
			surfaceNode->position = positionOffset;
			surfaceNode->material = material;
			surfaceNode->materialHandler = mh;
			positionOffset += ImVec2(60, 60);
			nodes.push_back(surfaceNode);
			linkManager.links.push_back(Link(linkID++, surfaceNode->id, output->id, surfaceNode->outAttr[0], output->inAttr[0]));
			
			for (int i = 0; i < TextureName::TextureNameCount; i++) {
				if (material->textures[i]) {
					ImTextureNode* node = new ImTextureNode(++nodeIdStack, ++attIdStack);
					node->resourceID = material->textures[i]->resourceID;
					node->texHandler = Engine3D::getSelf()->renderer.createTexture(material->textures[i]);
					node->position = positionOffset;
					positionOffset += ImVec2(60, 60);
					nodes.push_back(node);

					//ImStandartSurfaceNode must follow the enum order in TextureName.
					linkManager.links.push_back(Link(linkID++, node->id, surfaceNode->id, node->outAttr[0], surfaceNode->inAttr[i - 1]));
				}
			}

		} else if (material->hasMedium()) {

		}
	}

	void ImTextureNode::render() {
		if (positionOnce) {
			ImNodes::SetNodeGridSpacePos(id, position);
			positionOnce = false;
		}
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Image Texture");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginOutputAttribute(outAttr[0]);
		ImGui::Text("Color");
		ImNodes::EndOutputAttribute();

		if (ImGui::Button("Open")) {
			const char* pathptr = tinyfd_openFileDialog(NodeEditor::openFile, NodeEditor::startingFolder, 0, NULL, NULL, 0);
			if (pathptr != nullptr) {
				textureFilePath = std::string(pathptr);
				resourceID = ResourceManager::getSelf()->loadTexture(textureFilePath, textureFilePath, true);
				texHandler = Engine3D::getSelf()->renderer.createTexture(ResourceManager::getSelf()->getTexture(resourceID));
			}
		}

		if (isHandleValid(texHandler.id))
			ImGui::Image(Engine3D::getSelf()->renderer.getTextureAPIID(texHandler), ImVec2(128, 128));

		ImNodes::EndNode();
	}

	void ImTextureNode::process(NodeEditor& nodeEditor) {
		ImNodeMemoryBlock memBlock = outValues.at(outAttr[0]);
		uint32_t* outValue = (uint32_t*)memBlock.memBuffer.data;
		*outValue = this->resourceID;
	}

	void ImStandartSurfaceNode::render() {
		if (positionOnce) {
			ImNodes::SetNodeGridSpacePos(id, position);
			positionOnce = false;
		}
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.blue[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.blue[1]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.blue[2]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Standart Surface BRDF");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginInputAttribute(inAttr[0]);
		ImGui::PushItemWidth(100);
		ImGui::Text("Albedo");
		ImGui::SameLine();
		if(linkManager->checkIfOutputIsLinked(inAttr[0]).isValid())
			ImGuiExt::ColorPicker("##albedoColor", &albedoColor[0], ImVec2(100, 25), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[1]);
		ImGui::Text("Roughness");
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[2]);
		ImGui::PushItemWidth(100);
		ImGui::Text("Metallic");
		ImGui::SameLine();
		if (!linkManager->checkIfOutputIsLinked(inAttr[2]).isValid())
			ImGuiExt::ColorPicker("##metallicColor", &metallicColor[0], ImVec2(100, 25), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[3]);
		ImGui::Text("Emission");
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[4]);
		ImGui::Text("Normal");
		ImNodes::EndInputAttribute();


		ImNodes::BeginInputAttribute(inAttr[5]);
		ImGui::Text("AO");
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[6]);
		ImGui::Text("Sheen");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat("##sheenFactor", &sheenFactor, 0.001f, 0.0, 1.0, "%.2f");
		ImNodes::EndInputAttribute();

		ImNodes::BeginOutputAttribute(outAttr[0]);
		ImGui::Text("Color");
		ImNodes::EndOutputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}
	
	void ImStandartSurfaceNode::process(NodeEditor& nodeEditor) {
		Link link0 = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[0]);

		if (link0.isValid()) {
			UniformType::Enum type = nodeEditor.getNode(link0.inNode)->getOutputType(link0.in);

			if (type == UniformType::Sampler) {
				ImNode* node = nodeEditor.getNode(link0.inNode);

				node->process(nodeEditor);

				uint32_t *result = (uint32_t*)node->outValues.at(link0.in).memBuffer.data;
				Texture* p = ResourceManager::getSelf()->getTexture(*result);
				p->textureName = TextureName::ALBEDO;
				//If the texture changed, create new Renderer API Tex
				if (p->resourceID != material->getTexture(TextureName::ALBEDO)->resourceID) {
					//TODO Engine3D::getSelf()->renderer.deleteTexture();
					materialHandler->textures[ctz(TextureName::ALBEDO)] = Engine3D::getSelf()->renderer.createTexture(p);
				}
				material->addTexture(TextureName::ALBEDO, p);
			}
		}

		Link link1 = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[1]);

		if (link1.isValid()) {
			UniformType::Enum type = nodeEditor.getNode(link1.inNode)->getOutputType(link1.in);

			if (type == UniformType::Sampler) {
				ImNode* node = nodeEditor.getNode(link1.inNode);

				node->process(nodeEditor);

				uint32_t* result = (uint32_t*)node->outValues.at(link1.in).memBuffer.data;
				material->addTexture(TextureName::ROUGHNESS, ResourceManager::getSelf()->getTexture(*result));
			}
		}
	}

	void ImMaterialOutputNode::render() {
		if (positionOnce) {
			ImNodes::SetNodeGridSpacePos(id, position);
			positionOnce = false;
		}
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Material Output");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginInputAttribute(inAttr[0]);
		ImGui::Text("Surface");
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[1]);
		ImGui::Text("Volume");
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	void ImMaterialOutputNode::process(NodeEditor& nodeEditor) {
	}

	void ImSplitRGBNode::render() {
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("RGB Split");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginInputAttribute(inAttr[0]);
		ImGui::Text("Color");
		ImNodes::EndInputAttribute();

		ImNodes::BeginOutputAttribute(outAttr[0]);
		ImGui::Text("R");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginOutputAttribute(outAttr[1]);
		ImGui::Text("G");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginOutputAttribute(outAttr[2]);
		ImGui::Text("B");
		ImNodes::EndOutputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	void ImSplitRGBNode::process(NodeEditor &nodeEditor) {
		Link link = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[0]);
		//If the link exists and the attribute linked is of type Vec3, proceed.
		if (link.isValid() && nodeEditor.getNode(link.inNode)->getOutputType(link.in) == UniformType::Vec3) {
			ImNode* node = nodeEditor.getNode(link.inNode);

			//Process all previous nodes in this chain until a node with no input, i.e. data node (leaf), is found.
			node->process(nodeEditor);

			glm::vec3* result = (glm::vec3*)node->outValues.at(link.in).memBuffer.data;
			
			//Writes the result according to this node's purpose.
			ImNodeMemoryBlock memBlock = outValues.at(outAttr[0]);
			float *outValue = (float*)memBlock.memBuffer.data;
			*outValue = result->x;
			
			memBlock = outValues.at(outAttr[1]);
			outValue = (float*)memBlock.memBuffer.data;
			*outValue = result->y;
			
			memBlock = outValues.at(outAttr[2]);
			outValue = (float*)memBlock.memBuffer.data;
			*outValue = result->z;
		}
	}

	void ImVec3Node::render() {
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Vector 3f");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginOutputAttribute(outAttr[0]);
		ImGui::Text("RBG");
		ImGui::PushItemWidth(160.0f);
		ImGui::DragFloat3("", (float*)(outValues.at(outAttr[0]).memBuffer.data), 0.1, -INFINITY, INFINITY, "%.2f");
		ImGui::PopItemWidth();
		ImNodes::EndOutputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	void ImVec3Node::process(NodeEditor &nodeEditor) {

	}

	void ImDebugValueNode::render() {
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Debug Value");
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginInputAttribute(inAttr[0]);
		ImGui::Text(value.c_str());
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	void ImDebugValueNode::process(NodeEditor& nodeEditor) {
		Link link = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[0]);
		if (link.isValid()) {
			ImNode* node = nodeEditor.getNode(link.inNode);

			node->process(nodeEditor);
			UniformType::Enum type = nodeEditor.getNode(link.inNode)->getOutputType(link.in);

			if (type == UniformType::Vec3) {
				glm::vec3* result = (glm::vec3*)node->outValues.at(link.in).memBuffer.data;
				value = toString(*result);
			} else if(type == UniformType::Float) {
				float* result = (float*)node->outValues.at(link.in).memBuffer.data;
				value = std::to_string(*result);
			}
		}
	}

	void ImMathOperationNode::render() {
		ImNodes::PushColorStyle(ImNodesCol_TitleBar, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, colorToInt32(gUIPalette.iceGrey[0]));
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted("Math Operation");
		ImNodes::EndNodeTitleBar();

		ImGui::PushItemWidth(120.0f);
		ImGui::Combo("", (int*)&currentOperation, MathOperation::name, IM_ARRAYSIZE(MathOperation::name));
		ImGui::PopItemWidth();

		ImNodes::BeginInputAttribute(inAttr[0]);
		ImGui::Text("Value 1");
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inAttr[1]);
		ImGui::Text("Value 2");
		ImNodes::EndInputAttribute();

		ImNodes::BeginOutputAttribute(outAttr[0]);
		ImGui::Text("Output");
		ImNodes::EndOutputAttribute();

		ImNodes::EndNode();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
		ImNodes::PopColorStyle();
	}

	void ImMathOperationNode::process(NodeEditor& nodeEditor) {
		Link link1 = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[0]);
		Link link2 = nodeEditor.linkManager.checkIfOutputIsLinked(inAttr[1]);

		if (link1.isValid() && MathOperation::isOneInputOperation(currentOperation)) {
		} else if (link1.isValid() && link2.isValid() && !MathOperation::isOneInputOperation(currentOperation)) {
			ImNode* node1 = nodeEditor.getNode(link1.inNode);
			node1->process(nodeEditor);
			UniformType::Enum type1 = nodeEditor.getNode(link1.inNode)->getOutputType(link1.in);

			ImNode* node2 = nodeEditor.getNode(link2.inNode);
			node2->process(nodeEditor);
			UniformType::Enum type2 = nodeEditor.getNode(link2.inNode)->getOutputType(link2.in);

			if (type1 == UniformType::Vec3 && type2 == UniformType::Vec3) {
				glm::vec3* result1 = (glm::vec3*)node1->outValues.at(link1.in).memBuffer.data;
				glm::vec3* result2 = (glm::vec3*)node2->outValues.at(link2.in).memBuffer.data;
				//Update the actual type stored in the output attribute.
				outValues.at(outAttr[0]).dataType = UniformType::Vec3;

				if (currentOperation == MathOperation::ADD) {
					ImNodeMemoryBlock memBlock = outValues.at(outAttr[0]);
					glm::vec3 *outValue = (glm::vec3*)memBlock.memBuffer.data;
					*outValue = (*result1) + (*result2);
				}
			}
		}
	}
}
