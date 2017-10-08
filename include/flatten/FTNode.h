#ifndef _FLATTEN_NODE_H_
#define _FLATTEN_NODE_H_

#include <cu/cu_macro.h>
#include <sprite2/s2_typedef.h>

#include <cstdint>
#include <variant>

namespace ft
{

class FTNode
{
public:
	FTNode() { Init(); }
	FTNode(const FTNode&) = delete;
	void operator=(const FTNode&) = delete;

	void Init(const s2::SprConstPtr& spr) 
	{
		Init();

		m_data = spr;
		SetDataSpr(true);
	}

	void Init(const s2::ActorConstPtr& actor)
	{
		Init();

		m_data = actor;
		SetDataSpr(false);
	}

	s2::SprConstPtr GetSpr() const {
		return IsDataSpr() ? std::get<0>(m_data).lock() : nullptr;
	}
	s2::ActorConstPtr GetActor() const {
		return IsDataSpr() ? nullptr : std::get<1>(m_data).lock();
	}

	int GetCount() const { return m_count; }

private:
	void Init()
	{
		m_parent = INVALID_ID;
		m_count = 0;
		m_layer = 0;
		m_flags = 0;
		m_dlist_pos   = 0;
		m_dlist_count = 0;

		SetDrawlistDirty(true);
	}

private:
	static const uint16_t FLAG_DATA_SPR       = 0x0001;
	static const uint16_t FLAG_UPDATE_DIRTY   = 0x0002;
	static const uint16_t FLAG_DRAWLIST_DIRTY = 0x0004;

public:
	CU_FLAG_METHOD(DataSpr, FLAG_DATA_SPR)
	CU_FLAG_METHOD(UpdateDirty, FLAG_UPDATE_DIRTY)
	CU_FLAG_METHOD(DrawlistDirty, FLAG_DRAWLIST_DIRTY)

private:
	static const uint16_t INVALID_ID = 0xffff;

private:
	std::variant<
		std::weak_ptr<const s2::Sprite>, 
		std::weak_ptr<const s2::Actor>
	> m_data;

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