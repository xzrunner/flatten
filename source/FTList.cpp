#include "flatten/FTList.h"
#include "flatten/FTNode.h"
#include "flatten/BuildListVisitor.h"
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

#include <memmgr/MemoryPool.h>
#include <cooking/DisplayList.h>

#include <assert.h>

namespace ft
{

FTList::FTList(s2::Actor* root)
	: m_root(root)
	, m_nodes(nullptr)
	, m_nodes_sz(0)
	, m_nodes_cap(0)
	, m_max_layer(0)
	, m_dirty(true)
{
}

FTList::~FTList()
{
	if (m_nodes) {
		free(m_nodes);
	}
}

bool FTList::Update(int pos, bool force, std::shared_ptr<cooking::DisplayList> dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return false;
	}

	assert(m_nodes[0].m_count == m_nodes_sz);

	bool ret = false;
	const FTNode* node_ptr = &m_nodes[pos];
	for (int i = 0, n = node_ptr->m_count; i < n; )
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

void FTList::DrawForward(int pos, const s2::RenderParams& rp)
{
	if (!CheckFirst(pos, nullptr)) {
		return;
	}

	sm::Matrix2D stk_mat[MAX_LAYER];
	s2::RenderColor stk_col[MAX_LAYER];
	int stk_sz = 0;

	sm::Matrix2D prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;

	s2::RenderParams* rp_child = s2::RenderParamsPool::Instance()->Pop();
	*rp_child = rp;

	assert(m_nodes[0].m_count == m_nodes_sz);

	const FTNode* node_ptr = &m_nodes[pos];
	int start_layer = node_ptr->m_layer;
	int prev_layer = start_layer - 1;
	for (int i = 0, n = node_ptr->m_count; i < n; )
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
			while (stk_sz > node_ptr->m_layer + 1 - start_layer) {
				--stk_sz;
			}
		}
		assert(node_ptr->m_layer + 1 - start_layer == stk_sz);

		s2::Utility::PrepareMat(stk_mat[stk_sz - 1], spr, actor, prev_mt);
		s2::Utility::PrepareColor(stk_col[stk_sz - 1], spr, actor, prev_col);
		prev_layer = node_ptr->m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = actor;
		if (spr->GetSymbol()->DrawFlatten(nullptr, *rp_child, spr)) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
		} else {
			++i;
			++node_ptr;
		}
	}

	s2::RenderParamsPool::Instance()->Push(rp_child);
}

static int COUNT = 0;

void FTList::DrawDeferred(int pos, const s2::RenderParams& rp,
	                      std::shared_ptr<cooking::DisplayList>& dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return;
	}

	FTNode* node_ptr = &m_nodes[pos];
	int node_count = node_ptr->m_count;

	++COUNT;

	if (dlist && !dlist->Empty() && COUNT > 5000) 
	{
		int d_begin, d_end;
		if (pos == 0) {
			d_begin = d_end = -1;
		} else {
			d_begin = node_ptr->m_dlist_pos;
			d_end = d_begin;
			for (int i = pos; i < node_count; ++i, ++node_ptr) {
				d_end += node_ptr->m_dlist_count;
			}
		}
		if (d_begin < 0 || d_begin < d_end) {
			dlist->Replay(d_begin, d_end);
		}
		return;
	}

	//if (COUNT > 5000) {
	//	return;
	//}

	if (COUNT == 5000) {
		int zz = 0;
	}

	sm::Matrix2D stk_mat[MAX_LAYER];
	s2::RenderColor stk_col[MAX_LAYER];
	int stk_sz = 0;

	sm::Matrix2D prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;

	s2::RenderParams* rp_child = s2::RenderParamsPool::Instance()->Pop();
	*rp_child = rp;

	assert(m_nodes[0].m_count == m_nodes_sz);

	cooking::DisplayList dlist_tmp(mm::MemoryPool::Instance()->GetFreelistAlloc());
	node_ptr = &m_nodes[pos];
	int start_layer = node_ptr->m_layer;
	int prev_layer = start_layer - 1;
	for (int i = 0, n = node_ptr->m_count; i < n; )
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
			while (stk_sz > node_ptr->m_layer + 1 - start_layer) {
				--stk_sz;
			}
		}
		assert(node_ptr->m_layer + 1 - start_layer == stk_sz);

		s2::Utility::PrepareMat(stk_mat[stk_sz - 1], spr, actor, prev_mt);
		s2::Utility::PrepareColor(stk_col[stk_sz - 1], spr, actor, prev_col);
		prev_layer = node_ptr->m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = actor;

		node_ptr->m_dlist_pos = static_cast<uint16_t>(dlist_tmp.Size());
		bool draw = spr->GetSymbol()->DrawFlatten(&dlist_tmp, *rp_child, spr);
		node_ptr->m_dlist_count = static_cast<uint16_t>(dlist_tmp.Size()) - node_ptr->m_dlist_pos;

		if (draw) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
		} else {
			++i;
			++node_ptr;
		}
	}

	s2::RenderParamsPool::Instance()->Push(rp_child);

	if (dlist && pos == 0) {
		*dlist = std::move(dlist_tmp);
//		dlist->Replay();
	}

	dlist_tmp.Replay(-1, -1);
}

void FTList::SetFrame(int pos, bool force, int frame,
	                  std::shared_ptr<cooking::DisplayList> dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return;
	}

	s2::UpdateParams params;

	assert(m_nodes[0].m_count == m_nodes_sz);

	const FTNode* node_ptr = &m_nodes[pos];
	for (int i = 0, n = node_ptr->m_count; i < n; )
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

void FTList::Build(std::shared_ptr<cooking::DisplayList> dlist)
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
		m_nodes = (FTNode*)realloc(m_nodes, sizeof(FTNode) * count);
	}
	m_nodes_cap = count;
	m_nodes_sz = 0;
	
	s2::SprVisitorParams params;
	params.actor = m_root;

	BuildListVisitor visitor(shared_from_this(), dlist);
	m_root->GetSpr()->Traverse(visitor, params);

	InitNeedUpdateFlag();

	assert(m_max_layer < MAX_LAYER);

	m_dirty = false;
}

void FTList::InitNeedUpdateFlag()
{
	if (m_nodes_sz == 0) {
		return;
	}

	FTNode* node_ptr = &m_nodes[m_nodes_sz - 1];
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
			while (parent != FTNode::INVALID_ID)
			{
				assert(parent >= 0 && parent < m_nodes_sz);
				FTNode& curr = m_nodes[parent];
				curr.SetNeedUpdate(true);
				parent = curr.m_parent;
			}
		}
	}
}

bool FTList::CheckFirst(int pos, std::shared_ptr<cooking::DisplayList> dlist)
{
	if (pos < 0) {
		return false;
	}
	if (m_dirty) {
		Build(dlist);
	}
	if (m_nodes_sz == 0 || pos >= m_nodes_sz) {
		return false;
	}
	return true;
}

}