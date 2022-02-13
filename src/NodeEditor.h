#pragma once
#include <vector>
#include <list>
#include "imnodes.h" 
#include "io/InputManager.h"
#include "utils/StringID.h"
#include "core/Renderer.h"
#include "utils/ColorPalette.h"
#include <algorithm>
#include <glm/glm.hpp>
#define INVALID_IM_ID std::numeric_limits<uint32_t>::max()

namespace narvalengine {
	/**
	 * NodeEditor based off ImNodes addon.
	 */
	typedef uint32_t ImNodeID;
	typedef uint32_t ImNodeAttrID;
	typedef uint32_t ImLinkID;
	class NodeEditor;

	struct Link {
		ImLinkID id = INVALID_IM_ID;
		ImNodeID inNode = INVALID_IM_ID;
		ImNodeID outNode = INVALID_IM_ID;
		ImNodeAttrID in = INVALID_IM_ID;
		ImNodeAttrID out = INVALID_IM_ID;

		Link(ImLinkID id) {
			this->id = id;
			in = INVALID_IM_ID;
			out = INVALID_IM_ID;
			inNode = INVALID_IM_ID;
			outNode = INVALID_IM_ID;
		}

		Link(ImLinkID id, ImNodeID inNode, ImNodeID outNode, ImNodeAttrID in, ImNodeAttrID out) {
			this->inNode = inNode;
			this->outNode = outNode;
			this->id = id;
			this->in = in;
			this->out = out;
		}

		bool operator==(const Link& other) const {
			if (id == other.id)
				return true;
			return false;
		}

		bool isValid() {
			if (id != INVALID_IM_ID)
				return true;
			return false;
		}
	};

	struct LinkManager {
		std::list<Link> links;

		LinkManager() {
		}

		~LinkManager() {
		}

		//TODO return the inLinked
		Link checkIfOutputIsLinked(ImNodeAttrID output) {
			for (std::list<Link>::iterator it = links.begin(); it != links.end(); ++it)
				if (it->out == output)
					return *it;
			return Link(INVALID_IM_ID);
		}
	};

	struct ImNodeMemoryBlock {
		UniformType::Enum dataType;
		MemoryBuffer memBuffer;

		ImNodeMemoryBlock(UniformType::Enum dataType, MemoryBuffer memBuffer) {
			this->dataType = dataType;
			this->memBuffer = memBuffer;
		}

		void free(){
			delete memBuffer.data;
		}
	};


	struct ImNode {
		ImNodeID id;
		uint32_t inAttrSize = 0;
		ImNodeAttrID inAttr[10]{};
		uint32_t outAttrSize = 0;
		ImNodeAttrID outAttr[10]{};
		std::unordered_map<ImNodeAttrID, ImNodeMemoryBlock> outValues;
		ImVec2 position{0, 0};
		bool positionOnce = true;
		//Cached status only changes to true when a link is created or destroyed.
		bool cached = true;

		ImNode() {};

		ImNode(const ImNodeID id) {
			this->id = id;
		}

		ImNode(const ImNodeID id, ImVec2 position) {
			this->id = id;
			this->position = position;
		}

		~ImNode() {
			//TODO free all blocks in outValues.
		}

		bool isLeaf() {
			//A node can also be considered a leaf if all its input attributes are not linked.
			if (inAttrSize == 0)
				return true;
		}

		UniformType::Enum getOutputType(ImNodeAttrID id) {
			if (outValues.count(id) > 0)
				return outValues.at(id).dataType;
			return UniformType::Enum::Count;
		}

		void addInAttr(ImNodeAttrID id) {
			inAttr[inAttrSize++] = id;
		}

		void addOutAttr(ImNodeAttrID id) {
			outAttr[outAttrSize++] = id;
		}

		/**
		 * Verifies if this node contains the Node attribute {@param id}.
		 * 
		 * @param id to be verified.
		 * @return true if it contains the id in either in or out attribute. False otherwise.
		 */
		bool containsAttributeID(ImNodeAttrID id) {
			for (int i = 0; i < inAttrSize; i++) 
				if (id == inAttr[i])
					return true;

			for (int i = 0; i < outAttrSize; i++) 
				if (id == outAttr[i])
					return true;

			return false;
		}

		/**
		 * Process all inputs into its output attributes.
		 * 
		 * @param nodeEditor
		 * @return glm::vec4 processed. 
		 */
		virtual void process(NodeEditor& nodeEditor) = 0;

		virtual void render() = 0;
	};
 
	struct ImTextureNode : public ImNode {
		std::string textureFilePath;
		StringID resourceID;
		TextureHandler texHandler = { INVALID_HANDLE };

		ImTextureNode(const ImNodeID id, ImNodeAttrID out1) {
			this->id = id;
			addOutAttr(out1);

			MemoryBuffer mem = { new uint32_t(), sizeof(uint32_t) };
			ImNodeMemoryBlock memBlock = ImNodeMemoryBlock(UniformType::Sampler, mem);
			outValues.insert({ out1, memBlock });
		};

		~ImTextureNode() {
			//free texHandler resource
		}

		void render();
		void process(NodeEditor& nodeEditor);
	};

	struct ImStandartSurfaceNode : public ImNode {
		Material* material;
		MaterialHandler *materialHandler;
		glm::vec3 albedoColor{1,1,1};
		glm::vec3 metallicColor{1,1,1};
		float sheenFactor = 0;
		LinkManager *linkManager;

		ImStandartSurfaceNode(const ImNodeID id, ImNodeAttrID in1, ImNodeAttrID in2, ImNodeAttrID in3, ImNodeAttrID in4, ImNodeAttrID in5, ImNodeAttrID in6, ImNodeAttrID out1, LinkManager *nodeLinks) {
			this->id = id;
			this->linkManager = nodeLinks;
			addInAttr(in1);
			addInAttr(in2);
			addInAttr(in3);
			addInAttr(in4);
			addInAttr(in5);
			addInAttr(in6);
			addOutAttr(out1);
		};

		void render();
		void process(NodeEditor& nodeEditor);
	};

	struct ImVolumeNode : public ImNode {
	};

	struct ImMaterialOutputNode : public ImNode {

		ImMaterialOutputNode(const ImNodeID id, ImNodeAttrID in1, ImNodeAttrID in2) {
			this->id = id;
			addInAttr(in1);
			addInAttr(in2);
		};

		void render();
		void process(NodeEditor& nodeEditor);
	};

	struct ImSplitRGBNode : public ImNode {

		ImSplitRGBNode(const ImNodeID id, ImNodeAttrID in1, ImNodeAttrID out1, ImNodeAttrID out2, ImNodeAttrID out3) {
			this->id = id;
			addInAttr(in1);
			addOutAttr(out1);
			addOutAttr(out2);
			addOutAttr(out3);

			MemoryBuffer mem = {new float(), sizeof(float)};
			ImNodeMemoryBlock memBlock = ImNodeMemoryBlock(UniformType::Float, mem);
			outValues.insert({ out1, memBlock });

			mem = {new float(), sizeof(float) };
			memBlock = ImNodeMemoryBlock(UniformType::Float, mem);
			outValues.insert({ out2, memBlock });

			mem = {new float(), sizeof(float) };
			memBlock = ImNodeMemoryBlock(UniformType::Float, mem);
			outValues.insert({ out3, memBlock });
		};

		void render();
		void process(NodeEditor &nodeEditor);
	};

	struct ImVec3Node : public ImNode {

		ImVec3Node(const ImNodeID id, ImNodeAttrID out1) {
			this->id = id;
			addOutAttr(out1);

			MemoryBuffer mem = { new glm::vec3(), sizeof(glm::vec3) };
			ImNodeMemoryBlock memBlock = ImNodeMemoryBlock(UniformType::Vec3, mem);
			outValues.insert({ out1, memBlock });
		};

		void render();
		void process(NodeEditor& nodeEditor);
	};

	struct ImDebugValueNode : public ImNode {
		std::string value = "";

		ImDebugValueNode(const ImNodeID id, ImNodeAttrID in1) {
			this->id = id;
			addInAttr(in1);
		};

		void render();
		void process(NodeEditor& nodeEditor);
	};

	struct MathOperation {
		enum Operation {
			ADD,
			MULTIPLY,
			Count
		};

		static inline const char* const name[] = {
			"Add",
			"Multiply"
		};

		static bool isOneInputOperation(MathOperation::Operation op) {
			if (MathOperation::ADD || MathOperation::MULTIPLY)
				return false;
			return true;
		}
	};

	struct ImMathOperationNode : public ImNode {
		MathOperation::Operation currentOperation;

		ImMathOperationNode(const ImNodeID id, ImNodeAttrID in1, ImNodeAttrID in2, ImNodeAttrID out1) {
			this->id = id;
			addInAttr(in1);
			addInAttr(in2);
			addOutAttr(out1);

			//Allocate for the biggest size we will operate on (128bits).
			MemoryBuffer mem = { new glm::vec4(), sizeof(glm::vec4) };
			ImNodeMemoryBlock memBlock = ImNodeMemoryBlock(UniformType::Vec4, mem);
			outValues.insert({ out1, memBlock });
		};

		void render();
		void process(NodeEditor& nodeEditor);
	};

	class NodeEditor {
	private:
		int startAttr = INVALID_IM_ID, endAttr = INVALID_IM_ID;
		int linkIDTemp = INVALID_IM_ID;
	public:
		//ImNodes
		ImNodeID nodeIdStack = 0;
		ImNodeAttrID attIdStack = 0;
		ImLinkID linkID = 0;
		LinkManager linkManager{};
		static constexpr char* openFile = "Open file";
		static constexpr char* startingFolder = "./";
		ImVec2 positionOffset{0, 0};

		std::vector<ImNode*> nodes;

		NodeEditor() {
			linkManager = LinkManager();
		};

		~NodeEditor();

		void loadFromMaterial(Material* material, MaterialHandler* mh);

		ImNodeID findNodeThatHasAttribute(ImNodeAttrID attID) {
			for (auto node : nodes) {
				if (node->containsAttributeID(attID)) {
					return node->id;
				}
			}
			return INVALID_IM_ID;
		}

		ImNode* getNode(ImNodeID id) {
			for (auto node : nodes)
				if (node->id == id)
					return node;
			return nullptr;
		}

		void update() {
			for (auto node : nodes) {
				node->process(*this);
			}
		}

		void render() {
			if (InputManager::getSelf()->eventTriggered("NODE_EDITOR_ADD_NEW_NODE")) 
				ImGui::OpenPopup("ImCreateNewNodePopUp");

			if (ImGui::BeginPopup("ImCreateNewNodePopUp")) {
				if (ImGui::Selectable("Texture"))
					nodes.push_back(new ImTextureNode(++nodeIdStack, ++attIdStack));
				if (ImGui::Selectable("Standart Surface BRDF"))
					nodes.push_back(new ImStandartSurfaceNode(++nodeIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack, &linkManager));
				if (ImGui::Selectable("Material Output"))
					nodes.push_back(new ImMaterialOutputNode(++nodeIdStack, ++attIdStack, ++attIdStack));
				if (ImGui::Selectable("RGB Split"))
					nodes.push_back(new ImSplitRGBNode(++nodeIdStack, ++attIdStack, ++attIdStack, ++attIdStack, ++attIdStack));
				if (ImGui::Selectable("Vec3"))
					nodes.push_back(new ImVec3Node(++nodeIdStack, ++attIdStack));
				if (ImGui::Selectable("Math Operation"))
					nodes.push_back(new ImMathOperationNode(++nodeIdStack, ++attIdStack, ++attIdStack, ++attIdStack));
				#ifdef NE_DEBUG_MODE
				if (ImGui::Selectable("Debug Value"))
					nodes.push_back(new ImDebugValueNode(++nodeIdStack, ++attIdStack));
				#endif
				ImGui::EndPopup();
			}

			ImNodes::BeginNodeEditor();
			ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

			for (int i = 0; i < nodes.size(); i++)
				nodes.at(i)->render();

			//Render the currently existing links.
			for (std::list<Link>::iterator it = linkManager.links.begin(); it != linkManager.links.end(); ++it)
				ImNodes::Link(it->id, it->in, it->out);

			ImNodes::EndNodeEditor();

			if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
				ImNodeID start = findNodeThatHasAttribute(startAttr);
				ImNodeID out = findNodeThatHasAttribute(endAttr);
				linkManager.links.push_back(Link(linkID++, start, out, startAttr, endAttr));
			}
			
			if (ImNodes::IsLinkDestroyed(&linkIDTemp)) 
				linkManager.links.remove({ (ImLinkID)linkIDTemp });
		}
	};
}
