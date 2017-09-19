#include "flatten/Flatten.h"
#include "flatten/BuildFlattenVisitor.h"
#include "flatten/CountNodesVisitor.h"

#include <sprite2/SprVisitorParams.h>
#include <sprite2/S2_Actor.h>
#include <sprite2/S2_Sprite.h>
#include <sprite2/RenderParams.h>
#include <sprite2/Utility.h>

#include <sprite2/S2_Symbol.h>
#include <sprite2/SymType.h>
#include <sprite2/DrawNode.h>
#include <sprite2/AnimSprite.h>
#include <sprite2/UpdateParams.h>

#include <assert.h>

namespace ft
{

Flatten::Flatten(s2::Actor* root)
	: m_root(root)
	, m_nodes(nullptr)
	, m_nodes_sz(0)
	, m_nodes_cap(0)
	, m_max_layer(0)
{
	Build();
}

Flatten::~Flatten()
{
	if (m_nodes) {
		free(m_nodes);
	}
}

bool Flatten::Update(bool force)
{
	if (m_nodes_sz == 0) {
		return false;
	}

	bool ret = false;
	const Node* node_ptr = &m_nodes[0];
	for (int i = 0; i < m_nodes_sz; )
	{
		if (!node_ptr->IsNeedUpdate()) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
			continue;
		}

		// todo: up from root

		if (node_ptr->IsDataSpr()) 
		{
			const s2::Sprite* spr = static_cast<const s2::Sprite*>(node_ptr->m_data);
			if ((!force && !spr->IsInheritUpdate()) ||
				!spr->IsVisible()) {
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} else {
				const_cast<s2::Sprite*>(spr)->AutoUpdate(nullptr);
				++i;
				++node_ptr;
			}
		} 
		else 
		{
			const s2::Actor* actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			if ((!force && !actor->GetSpr()->IsInheritUpdate()) ||
				!actor->IsVisible()) {
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} else {
				const_cast<s2::Sprite*>(actor->GetSpr())->AutoUpdate(actor);
				++i;
				++node_ptr;
			}
		}		
	}
	return ret;
}

void Flatten::Draw(const s2::RenderParams& rp) const
{
	if (m_nodes_sz == 0) {
		return;
	}

	sm::Matrix2D stk_mat[MAX_LAYER];
	s2::RenderColor stk_col[MAX_LAYER];
	int stk_sz = 0;

	sm::Matrix2D prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;
	int prev_layer = -1;

	s2::RenderParams* rp_child = s2::RenderParamsPool::Instance()->Pop();
	*rp_child = rp;

	const Node* node_ptr = &m_nodes[0];
	for (int i = 0; i < m_nodes_sz; )
	{
		const s2::Sprite* spr = nullptr;
		const s2::Actor* actor = nullptr;

		if (node_ptr->IsDataSpr()) {
			spr = static_cast<const s2::Sprite*>(node_ptr->m_data);
		} else {
			actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			spr = actor->GetSpr();
		}

		bool visible = actor ? actor->IsVisible() : spr->IsVisible();
		if (!visible) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
			continue;
		}

		if (node_ptr->m_layer == prev_layer) {
			;
		} else if (node_ptr->m_layer == prev_layer + 1) {
			stk_mat[stk_sz] = prev_mt;
			stk_col[stk_sz] = prev_col;
			++stk_sz;
		} else {
			assert(node_ptr->m_layer < prev_layer);
			while (stk_sz > node_ptr->m_layer + 1) {
				--stk_sz;
			}
		}
		assert(node_ptr->m_layer + 1 == stk_sz);

		s2::Utility::PrepareMat(stk_mat[stk_sz - 1], spr, actor, prev_mt);
		s2::Utility::PrepareColor(stk_col[stk_sz - 1], spr, actor, prev_col);
		prev_layer = node_ptr->m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = actor;
		if (spr->GetSymbol()->DrawFlatten(*rp_child, spr)) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
		} else {
			++i;
			++node_ptr;
		}
	}

	s2::RenderParamsPool::Instance()->Push(rp_child);
}

void Flatten::SetFrame(bool force, int frame)
{
	if (m_nodes_sz == 0) {
		return;
	}

	s2::UpdateParams params;

	const Node* node_ptr = &m_nodes[0];
	for (int i = 0; i < m_nodes_sz; )
	{
		if (!node_ptr->IsNeedUpdate()) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
			continue;
		}

		if (node_ptr->IsDataSpr())
		{
			const s2::Sprite* spr = static_cast<const s2::Sprite*>(node_ptr->m_data);
			if ((!force && !spr->IsInheritUpdate()) ||
				!spr->IsVisible() ||
				spr->GetSymbol()->Type() != s2::SYM_ANIMATION) 
			{
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} 
			else 
			{
				const s2::AnimSprite* anim_spr = VI_DOWNCASTING<const s2::AnimSprite*>(spr);
				params.SetActor(nullptr);
				const_cast<s2::AnimSprite*>(anim_spr)->SetFrame(params, frame);
				++i;
				++node_ptr;
			}
		}
		else
		{
			const s2::Actor* actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			if ((!force && !actor->GetSpr()->IsInheritUpdate()) ||
				!actor->IsVisible() ||
				actor->GetSpr()->GetSymbol()->Type() != s2::SYM_ANIMATION) 
			{
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} 
			else 
			{
				const s2::AnimSprite* anim_spr = VI_DOWNCASTING<const s2::AnimSprite*>(actor->GetSpr());
				params.SetActor(actor);
				const_cast<s2::AnimSprite*>(anim_spr)->SetFrame(params, frame);
				++i;
				++node_ptr;
			}
		}
	}
}

void Flatten::Build()
{
	int count = 0;
	{
		s2::SprVisitorParams params;
		params.actor = m_root;

		CountNodesVisitor visitor;
		m_root->GetSpr()->Traverse(visitor, params);

		count = visitor.GetCount();
	}

	if (!m_nodes || m_nodes_cap != count) {
		m_nodes = (Node*)realloc(m_nodes, sizeof(Node) * count);
	}
	m_nodes_cap = count;
	m_nodes_sz = 0;
	
	s2::SprVisitorParams params;
	params.actor = m_root;

	BuildFlattenVisitor visitor(*this);
	m_root->GetSpr()->Traverse(visitor, params);

	InitNeedUpdateFlag();

	assert(m_max_layer < MAX_LAYER);
}

void Flatten::InitNeedUpdateFlag()
{
	if (m_nodes_sz == 0) {
		return;
	}

	Node* node_ptr = &m_nodes[m_nodes_sz - 1];
	for (int i = m_nodes_sz - 1; i >= 0; --i, --node_ptr)
	{
		if (node_ptr->IsNeedUpdate()) {
			continue;
		}
		assert(node_ptr->m_data);
		bool need_update = false;
		if (node_ptr->IsDataSpr()) {
			need_update = static_cast<const s2::Sprite*>(node_ptr->m_data)->NeedAutoUpdate(nullptr);
		} else {
			const s2::Actor* actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			need_update = actor->GetSpr()->NeedAutoUpdate(actor);
		}
		if (need_update) 
		{
			node_ptr->SetNeedUpdate(true);
			uint16_t parent = node_ptr->m_parent;
			while (parent != Node::INVALID_ID)
			{
				assert(parent >= 0 && parent < m_nodes_sz);
				Node& curr = m_nodes[parent];
				curr.SetNeedUpdate(true);
				parent = curr.m_parent;
			}
		}
	}
}

/************************************************************************/
/* class Flatten::Node                                                 */
/************************************************************************/

Flatten::Node::Node()
{
	Init();
}

void Flatten::Node::Init(const s2::Sprite* spr)
{
	Init();

	m_data = spr;
	SetDataSpr(true);
}

void Flatten::Node::Init(const s2::Actor* actor)
{
	Init();

	m_data = actor;
	SetDataSpr(false);
}

void Flatten::Node::Init()
{
	m_data = nullptr;
	m_parent = INVALID_ID;
	m_count = 0;
	m_layer = 0;
	m_flags = 0;
}

}