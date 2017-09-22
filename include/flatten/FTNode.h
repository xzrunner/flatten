#ifndef _FLATTEN_NODE_H_
#define _FLATTEN_NODE_H_

#include <sprite2/s2_macro.h>

#include <cstdint>

namespace s2 { class Sprite; class Actor; }

namespace ft
{

class FTNode
{
public:
	FTNode() { Init(); }
	FTNode(const FTNode&) = delete;
	void operator=(const FTNode&) = delete;

	void Init(const s2::Sprite* spr) 
	{
		Init();

		m_data = spr;
		SetDataSpr(true);
	}

	void Init(const s2::Actor* actor)
	{
		Init();

		m_data = actor;
		SetDataSpr(false);
	}

private:
	void Init()
	{
		m_data = nullptr;
		m_parent = INVALID_ID;
		m_count = 0;
		m_layer = 0;
		m_flags = 0;
		m_dlist_pos = 0;
		m_dlist_count = 0;
	}

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

	uint16_t m_dlist_pos;
	uint16_t m_dlist_count;

	friend class FTList;
	friend class BuildListVisitor;

}; // FTNode

}

#endif // _FLATTEN_NODE_H_