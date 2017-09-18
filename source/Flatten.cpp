#include "flatten/Flatten.h"
#include "flatten/BuildFlattenVisitor.h"

#include <sprite2/SprVisitorParams.h>
#include <sprite2/S2_Actor.h>
#include <sprite2/S2_Sprite.h>
#include <sprite2/RenderParams.h>
#include <sprite2/Utility.h>

#include <sprite2/S2_Symbol.h>
#include <sprite2/SymType.h>
#include <sprite2/DrawNode.h>

#include <assert.h>

namespace ft
{

Flatten::Flatten(s2::Actor* root)
	: m_root(root)
{
	Build();
}

bool Flatten::Update(bool force)
{
	bool ret = false;
	for (int i = 0, n = m_nodes.size(); i < n; )
	{
		Node& node = m_nodes[i];
		if (!node.IsNeedUpdate()) {
			i += node.m_count;
			continue;
		}

		// todo: up from root

		if (node.IsDataSpr()) 
		{
			const s2::Sprite* spr = static_cast<const s2::Sprite*>(node.m_data);
			if ((!force && !spr->IsInheritUpdate()) ||
				!spr->IsVisible()) {
				i += node.m_count;
			} else {
				const_cast<s2::Sprite*>(spr)->AutoUpdate(nullptr);
				++i;
			}
		} 
		else 
		{
			const s2::Actor* actor = static_cast<const s2::Actor*>(node.m_data);
			if ((!force && !actor->GetSpr()->IsInheritUpdate()) ||
				!actor->IsVisible()) {
				i += node.m_count;
			} else {
				const_cast<s2::Sprite*>(actor->GetSpr())->AutoUpdate(actor);
				++i;
			}
		}		
	}
	return ret;
}

void Flatten::Draw(const s2::RenderParams& rp) const
{
	std::stack<sm::Matrix2D> stk_mat;
	std::stack<s2::RenderColor> stk_col;

	sm::Matrix2D prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;
	int prev_layer = -1;

	s2::RenderParams* rp_child = s2::RenderParamsPool::Instance()->Pop();
	*rp_child = rp;

	for (int i = 0, n = m_nodes.size(); i < n; )
	{
		const Node& node = m_nodes[i];

		const s2::Sprite* spr = nullptr;
		const s2::Actor* actor = nullptr;

		if (node.IsDataSpr()) {
			spr = static_cast<const s2::Sprite*>(node.m_data);
		} else {
			actor = static_cast<const s2::Actor*>(node.m_data);
			spr = actor->GetSpr();
		}

		bool visible = actor ? actor->IsVisible() : spr->IsVisible();
		if (!visible) {
			i += node.m_count;
			continue;
		}

		if (node.m_layer == prev_layer) {
			;
		} else if (node.m_layer == prev_layer + 1) {
			stk_mat.push(prev_mt);
			stk_col.push(prev_col);
		} else {
			assert(node.m_layer < prev_layer && stk_mat.size() == stk_col.size());
			while (stk_mat.size() > node.m_layer + 1) {
				stk_mat.pop();
				stk_col.pop();
			}
		}
		assert(node.m_layer + 1 == stk_mat.size() && stk_mat.size() == stk_col.size());

		s2::Utility::PrepareMat(stk_mat.top(), spr, actor, prev_mt);
		s2::Utility::PrepareColor(stk_col.top(), spr, actor, prev_col);
		prev_layer = node.m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = actor;
		if (spr->GetSymbol()->DrawFlatten(*rp_child, spr)) {
			i += node.m_count;
		} else {
			++i;
		}
	}

	s2::RenderParamsPool::Instance()->Push(rp_child);
}

void Flatten::Build()
{
	m_nodes.clear();

	s2::SprVisitorParams params;
	params.actor = m_root;

	BuildFlattenVisitor visitor(*this);
	m_root->GetSpr()->Traverse(visitor, params);

	InitNeedUpdateFlag();
}

void Flatten::InitNeedUpdateFlag()
{
	if (m_nodes.empty()) {
		return;
	}

	for (int i = m_nodes.size() - 1; i >= 0; --i)
	{
		Node& node = m_nodes[i];
		if (node.IsNeedUpdate()) {
			continue;
		}
		assert(node.m_data);
		bool need_update = false;
		if (node.IsDataSpr()) {
			need_update = static_cast<const s2::Sprite*>(node.m_data)->NeedAutoUpdate(nullptr);
		} else {
			const s2::Actor* actor = static_cast<const s2::Actor*>(node.m_data);
			need_update = actor->GetSpr()->NeedAutoUpdate(actor);
		}
		if (need_update) 
		{
			node.SetNeedUpdate(true);
			uint16_t parent = node.m_parent;
			while (parent != Node::INVALID_ID)
			{
				assert(parent >= 0 && parent < m_nodes.size());
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

Flatten::Node::Node(const s2::Sprite* spr)
	: m_data(spr)
	, m_parent(INVALID_ID)
	, m_count(0)
	, m_layer(0)
	, m_flags(0)
{
	SetDataSpr(true);
}

Flatten::Node::Node(const s2::Actor* actor)
	: m_data(actor)
	, m_parent(INVALID_ID)
	, m_count(0)
	, m_layer(0)
	, m_flags(0)
{
	SetDataSpr(false);
}

}