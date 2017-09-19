#ifndef _FLATTEN_FLATTEN_H_
#define _FLATTEN_FLATTEN_H_

#include <sprite2/s2_macro.h>

#include <vector>
#include <memory>

#include <stdint.h>

namespace s2 { class Sprite; class Actor; class RenderParams; }

namespace ft
{

class Flatten : public std::enable_shared_from_this<Flatten>
{
public:
	Flatten(s2::Actor* root);
	Flatten(const Flatten&) = delete;
	Flatten(Flatten&&) = delete;
	~Flatten();

	bool Update(int pos, bool force);
	void Draw(int pos, const s2::RenderParams& rp);

	void SetFrame(int pos, bool force, int frame);

	void SetDirty() { m_dirty = true; }

private:
	void Build();

	void InitNeedUpdateFlag();

	bool CheckFirst(int pos);

private:
	class Node
	{
	public:
		Node();
		Node(const Node&) = delete;
		void operator=(const Node&) = delete;

		void Init(const s2::Sprite* spr);
		void Init(const s2::Actor* actor);

	private:
		void Init();

	private:
		static const uint16_t FLAG_DATA_SPR    = 0x0001;
		static const uint16_t FLAG_NEED_UPDATE = 0x0002;

	public:
		S2_FLAG_METHOD(DataSpr, FLAG_DATA_SPR)
		S2_FLAG_METHOD(NeedUpdate, FLAG_NEED_UPDATE)

	private:
		static const uint16_t INVALID_ID = 0xffff;

	private:
		const void* m_data;

		uint16_t m_parent;

		uint16_t m_count;
		uint16_t m_layer;
		
		mutable uint16_t m_flags;

		friend class Flatten;
		friend class BuildFlattenVisitor;

	}; // Node

private:
	static const int MAX_LAYER = 16;

private:
	s2::Actor* m_root;

	Node* m_nodes;
	int m_nodes_sz, m_nodes_cap;

	int m_max_layer;

	bool m_dirty;

	friend class BuildFlattenVisitor;

}; // Flatten

}

#endif // _FLATTEN_FLATTEN_H_
