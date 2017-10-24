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
#include <sprite2/ComplexSymbol.h>
#include <sprite2/RenderScissor.h>

#include <unirender/UR_RenderContext.h>
#include <cooking/DisplayList.h>
#include <memmgr/BlockAllocatorPool.h>
#include <shaderlab/ShaderMgr.h>
#include <shaderlab/FilterShader.h>

#include <assert.h>

namespace ft
{

static int TOT_MEM = 0;
static int TOT_COUNT = 0;
static int TOT_ALLOC = 0;
static int TOT_FREE = 0;

FTList::FTList(const s2::ActorPtr& root)
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
		--TOT_COUNT;
		++TOT_FREE;
		size_t sz = sizeof(FTNode) * m_nodes_cap;
		TOT_MEM -= sz;
//		printf("+++ free, mem %d, count %d, alloc% d, free %d\n", TOT_MEM, TOT_COUNT, TOT_ALLOC, TOT_FREE);

		mm::AllocHelper::Free(m_nodes, sz);
	}
}

bool FTList::Update(int pos, bool force, const std::shared_ptr<cooking::DisplayList>& dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return false;
	}

	assert(m_nodes[0].m_count == m_nodes_sz);

	bool ret = false;
	const FTNode* node_ptr = &m_nodes[pos];
	for (int i = 0, n = node_ptr->m_count; i < n; )
	{
		if (!node_ptr->IsUpdateDirty()) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
			continue;
		}

		// todo: up from root

		bool dirty = false;
		const FTNode* curr_node_ptr = node_ptr;

		if (node_ptr->IsDataSpr()) 
		{
			auto spr(static_cast<const s2::Sprite*>(node_ptr->m_data));
			if ((!force && !spr->IsInheritUpdate()) ||
				!spr->IsVisible()) {
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} else {
				dirty = const_cast<s2::Sprite*>(spr)->AutoUpdate(nullptr);
				++i;
				++node_ptr;
			}
		} 
		else 
		{
			auto actor(static_cast<const s2::Actor*>(node_ptr->m_data));
			if ((!force && !actor->GetSprRaw()->IsInheritUpdate()) ||
				!actor->IsVisible()) {
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} else {
				dirty = const_cast<s2::Sprite*>(actor->GetSprRaw())->AutoUpdate(actor);
				++i;
				++node_ptr;
			}
		}

		if (dirty) {
			SetDrawlistDirty(curr_node_ptr);
		}
	}
	return ret;
}

void FTList::DrawForward(int pos, const s2::RenderParams& rp)
{
	if (!CheckFirst(pos, nullptr)) {
		return;
	}

	sm::Matrix2D    stk_mat[MAX_LAYER];
	s2::RenderColor stk_col[MAX_LAYER];
	s2::FilterMode  stk_filter[MAX_LAYER];
	sm::rect        stk_scissor[MAX_LAYER];
	int stk_sz = 0;

	sm::Matrix2D    prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;
	s2::FilterMode  prev_filter = rp.render_filter;
	sm::rect        prev_scissor;

	s2::RenderParams* rp_child = static_cast<s2::RenderParams*>(mm::AllocHelper::Allocate(sizeof(s2::RenderParams)));
	memcpy(rp_child, &rp, sizeof(rp));

	assert(m_nodes[0].m_count == m_nodes_sz);

	sl::ShaderMgr* shader_mgr = sl::ShaderMgr::Instance();

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
			spr = actor->GetSprRaw();
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
			stk_mat[stk_sz]     = prev_mt;
			stk_col[stk_sz]     = prev_col;
			stk_filter[stk_sz]  = prev_filter;
			stk_scissor[stk_sz] = prev_scissor;
			++stk_sz;
		} else {
			assert(node_ptr->m_layer < prev_layer);
			while (stk_sz > node_ptr->m_layer + 1 - start_layer) {
				const sm::rect& scissor = stk_scissor[stk_sz - 1];
				if (scissor.IsValid()) {
					s2::RenderScissor::Instance()->Pop();
				}
				--stk_sz;
			}
		}
		assert(node_ptr->m_layer + 1 - start_layer == stk_sz);
		
		s2::Utility::PrepareMat(stk_mat[stk_sz - 1], spr, actor, prev_mt);
		s2::Utility::PrepareColor(stk_col[stk_sz - 1], spr, actor, prev_col);
		prev_filter  = stk_filter[stk_sz - 1];
		if (spr->GetSymbol()->Type() == s2::SYM_COMPLEX) {
			prev_scissor = S2_VI_PTR_DOWN_CAST<s2::ComplexSymbol>(spr->GetSymbol())->GetScissor();
			if (prev_scissor.Width() == 0 || prev_scissor.Height() == 0) {
				prev_scissor.MakeEmpty();
			}
		} else {
			prev_scissor.MakeEmpty();
		}
		prev_layer = node_ptr->m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = actor;

		PrepareDraw(shader_mgr, *rp_child, spr, prev_filter);
		rp_child->render_filter = prev_filter;
		
		if (prev_scissor.IsValid())
		{
			sm::vec2 min = rp_child->mt * sm::vec2(prev_scissor.xmin, prev_scissor.ymin),
				     max = rp_child->mt * sm::vec2(prev_scissor.xmax, prev_scissor.ymax);
			if (min.x > max.x) {
				std::swap(min.x, max.x);
			}
			if (min.y > max.y) {
				std::swap(min.y, max.y);
			}
			s2::RenderScissor::Instance()->Push(min.x, min.y, max.x - min.x, max.y - min.y, true, false);
		}
		if (spr->GetSymbol()->DrawNode(nullptr, *rp_child, spr, *this, i) == s2::RENDER_SKIP) {
			i++;
			node_ptr++;
		} else {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
		}

	}

	mm::AllocHelper::Free(rp_child, sizeof(s2::RenderParams));
}

void FTList::DrawDeferred(int pos, const s2::RenderParams& rp,
	                      const std::shared_ptr<cooking::DisplayList>& dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return;
	}

	sm::Matrix2D    stk_mat[MAX_LAYER];
	s2::RenderColor stk_col[MAX_LAYER];
	s2::FilterMode  stk_filter[MAX_LAYER];

	int stk_sz = 0;

	sm::Matrix2D    prev_mt = rp.mt;
	s2::RenderColor prev_col = rp.color;
	s2::FilterMode  prev_filter = rp.render_filter;

	s2::RenderParams* rp_child = static_cast<s2::RenderParams*>(mm::AllocHelper::Allocate(sizeof(s2::RenderParams)));
	memcpy(rp_child, &rp, sizeof(rp));

	assert(m_nodes[0].m_count == m_nodes_sz);

	cooking::DisplayList dlist_tmp;

	FTNode* node_ptr = &m_nodes[pos];

	// back up
	int old_pos = pos;
	int old_dlist_pos = node_ptr->m_dlist_pos;
	int old_dlist_count = CalcDListAllCount(*dlist, pos);

	sl::ShaderMgr* shader_mgr = sl::ShaderMgr::Instance();

	node_ptr = &m_nodes[pos];
	int pos_off = node_ptr->m_dlist_pos;
	int start_layer = node_ptr->m_layer;
	int prev_layer = start_layer - 1;
	for (int i = old_pos, n = old_pos + node_ptr->m_count; i < n; )
	{
		// only copy
		if (!node_ptr->IsDrawlistDirty()) 
		{
			int count = CalcDListAllCount(*dlist, i);
			if (count > 0) {
				dlist_tmp.DeepCopyFrom(*dlist, node_ptr->m_dlist_pos, count);
			}

			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;

			continue;
		}

		const s2::Sprite* spr = nullptr;
		const s2::Actor* actor = nullptr;

		if (node_ptr->IsDataSpr()) {
			spr = static_cast<const s2::Sprite*>(node_ptr->m_data);
		} else {
			actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			spr = actor->GetSprRaw();
		}

		bool visible = actor ? actor->IsVisible() : spr->IsVisible();
		if (!visible) {
			int sz = node_ptr->m_count;
			for (int n = i + sz; i < n; ++i, ++node_ptr) {
				node_ptr->m_dlist_count = 0;
			}
			continue;
		}

		if (node_ptr->m_layer == prev_layer) {
			;
		} else if (node_ptr->m_layer == prev_layer + 1) {
			stk_mat[stk_sz]    = prev_mt;
			stk_col[stk_sz]    = prev_col;
			stk_filter[stk_sz] = prev_filter;
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
		prev_filter = stk_filter[stk_sz - 1];
		prev_layer = node_ptr->m_layer;

		rp_child->mt = prev_mt;
		rp_child->color = prev_col;
		rp_child->actor = static_cast<const s2::Actor*>(node_ptr->m_data);

		PrepareDraw(shader_mgr, *rp_child, spr, prev_filter);
		rp_child->render_filter = prev_filter;

		int start = pos_off + static_cast<uint16_t>(dlist_tmp.Size());
		int old_count = node_ptr->m_dlist_count;
		s2::RenderReturn ret = spr->GetSymbol()->DrawNode(&dlist_tmp, *rp_child, spr, *this, i);
		node_ptr->m_dlist_count = pos_off + static_cast<uint16_t>(dlist_tmp.Size()) - start;

		node_ptr->SetDrawlistDirty(false);

		if (ret == s2::RENDER_SKIP) {
			i++;
			node_ptr++;
		} else {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
		}
	}

	mm::AllocHelper::Free(rp_child, sizeof(s2::RenderParams));

	if (dlist) 
	{
		// reset node's dlist_pos
		FTNode* node_ptr = &m_nodes[0];
		int pos = 0;
		for (int i = 0; i < m_nodes_sz; ++i, ++node_ptr) {
			node_ptr->m_dlist_pos = pos;
			pos += node_ptr->m_dlist_count;
		}

		int dlist_tmp_sz = dlist_tmp.Size();

		// rebuild dlist
		if (old_pos == 0) {
			*dlist = dlist_tmp;
		} else {
			cooking::DisplayList tmp;
			const ft::FTNode& node = m_nodes[old_pos];
			tmp.DeepCopyFrom(*dlist, 0, old_dlist_pos);
			tmp.DeepCopyFrom(dlist_tmp, 0, -1);
			tmp.DeepCopyFrom(*dlist, old_dlist_pos + old_dlist_count, -1);

			*dlist = tmp;
		}
		
		// draw
		if (dlist_tmp_sz > 0) {
			dlist->Replay(old_dlist_pos, old_dlist_pos + dlist_tmp_sz);
		}
	}
	else
	{
		dlist_tmp.Replay(-1, -1);
	}
}

void FTList::SetFrame(int pos, bool force, int frame,
	                  const std::shared_ptr<cooking::DisplayList>& dlist)
{
	if (!CheckFirst(pos, dlist)) {
		return;
	}

	s2::UpdateParams params;

	assert(m_nodes[0].m_count == m_nodes_sz);

	const FTNode* node_ptr = &m_nodes[pos];
	for (int i = 0, n = node_ptr->m_count; i < n; )
	{
		if (!node_ptr->IsUpdateDirty()) {
			i += node_ptr->m_count;
			node_ptr += node_ptr->m_count;
			continue;
		}

		bool dirty = false;
		const FTNode* curr_node_ptr = node_ptr;

		if (node_ptr->IsDataSpr())
		{
			auto spr(static_cast<const s2::Sprite*>(node_ptr->m_data));
			if ((!force && !spr->IsInheritUpdate()) ||
				!spr->IsVisible()) 
			{
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} 
			else 
			{
				auto type = spr->GetSymbol()->Type();
				if (type == s2::SYM_ANIMATION) {
					params.SetActor(nullptr);
					auto anim_spr = S2_VI_DOWN_CAST<const s2::AnimSprite*>(spr);
					dirty = const_cast<s2::AnimSprite*>(anim_spr)->SetFrame(params, frame);
				}
				++i;
				++node_ptr;
			}
		}
		else
		{
			auto actor(static_cast<const s2::Actor*>(node_ptr->m_data));
			if ((!force && !actor->GetSprRaw()->IsInheritUpdate()) ||
				!actor->IsVisible())
			{
				i += node_ptr->m_count;
				node_ptr += node_ptr->m_count;
			} 
			else 
			{
				auto type = actor->GetSprRaw()->GetSymbol()->Type();
				if (type == s2::SYM_ANIMATION) {
					params.SetActor(const_cast<s2::Actor*>(actor));
					auto anim_spr = S2_VI_DOWN_CAST<const s2::AnimSprite*>(actor->GetSprRaw());
					dirty = const_cast<s2::AnimSprite*>(anim_spr)->SetFrame(params, frame);
				}
				++i;
				++node_ptr;
			}
		}

		if (dirty) {
			SetDrawlistDirty(curr_node_ptr);
		}
	}
}

const FTNode* FTList::GetNode(int pos) const
{
	if (pos >= 0 && pos < m_nodes_sz) {
		return &m_nodes[pos];
	} else {
		return nullptr;
	}
}

void FTList::Build(const std::shared_ptr<cooking::DisplayList>& dlist)
{
	auto root = m_root.lock();
	if (!root) {
		return;
	}

	int count = 0;
	{
		s2::SprVisitorParams params;
		params.actor = root.get();

		CountNodesVisitor visitor;
		root->GetSprRaw()->Traverse(visitor, params);

		count = visitor.GetCount();
	}

	if (!m_nodes || m_nodes_cap != count) {

		if (!m_nodes) {
			++TOT_COUNT;
		}
		++TOT_ALLOC;
		TOT_MEM += (count - m_nodes_cap) * sizeof(FTNode);
//		printf("+++ alloc, mem %d, count %d, alloc% d, free %d, sz %d\n", TOT_MEM, TOT_COUNT, TOT_ALLOC, TOT_FREE, (count - m_nodes_cap) * sizeof(FTNode));

		if (m_nodes) {
			mm::AllocHelper::Free(m_nodes, sizeof(FTNode) * m_nodes_cap);
		}
		m_nodes = static_cast<FTNode*>(mm::AllocHelper::Allocate(sizeof(FTNode) * count));
	}
	m_nodes_cap = count;
	m_nodes_sz = 0;
	
	s2::SprVisitorParams params;
	params.actor = root.get();
	BuildListVisitor visitor(shared_from_this(), dlist);
	root->GetSprRaw()->Traverse(visitor, params);

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
		if (node_ptr->IsUpdateDirty()) {
			continue;
		}
		bool update_dirty = false;
		if (node_ptr->IsDataSpr()) {
			auto spr = static_cast<const s2::Sprite*>(node_ptr->m_data);
			update_dirty = spr->NeedAutoUpdate(nullptr);
		} else {
			auto actor = static_cast<const s2::Actor*>(node_ptr->m_data);
			update_dirty = actor->GetSprRaw()->NeedAutoUpdate(actor);
		}
		if (update_dirty) 
		{
			node_ptr->SetUpdateDirty(true);
			uint16_t parent = node_ptr->m_parent;
			while (parent != FTNode::INVALID_ID)
			{
				assert(parent >= 0 && parent < m_nodes_sz);
				FTNode& curr = m_nodes[parent];
				curr.SetUpdateDirty(true);
				parent = curr.m_parent;
			}
		}
	}
}

bool FTList::CheckFirst(int pos, const std::shared_ptr<cooking::DisplayList>& dlist)
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

int FTList::CalcDListAllCount(const cooking::DisplayList& dlist, int pos) const
{
	assert(pos >= 0 && pos < m_nodes_sz);

	int count = 0;
	if (pos == 0)
	{
		const FTNode& node = m_nodes[pos];
		assert(node.m_parent == FTNode::INVALID_ID && node.m_dlist_pos == 0);
		count = dlist.Size();
	}
	else
	{
		FTNode* node_ptr = &m_nodes[pos];
		int node_count = node_ptr->m_count;
		for (int i = 0; i < node_count; ++i, ++node_ptr) {
			count += node_ptr->m_dlist_count;
		}
	}
	return count;
}

void FTList::SetDrawlistDirty(const FTNode* node)
{
	// self & children
	const FTNode* ptr = node;
	for (int j = 0; j < node->m_count; ++j, ++ptr) {
		ptr->SetDrawlistDirty(true);
	}

	// parents
	uint16_t parent = node->m_parent;
	while (parent != FTNode::INVALID_ID) {
		const FTNode* ptr_parent = &m_nodes[parent];
		ptr_parent->SetDrawlistDirty(true);
		parent = ptr_parent->m_parent;
	}
}

void FTList::PrepareDraw(sl::ShaderMgr* shader_mgr, const s2::RenderParams& rp, 
	                     const s2::Sprite* spr, s2::FilterMode& filter)
{
	s2::RenderShader rs;
//	s2::RenderCamera rc;
	if (rp.IsDisableRenderDraw()) {
		rs = *s2::SprDefault::Instance()->Shader();
//		rc = *s2::SprDefault::Instance()->Camera();
	} else if (spr->HaveActor()) {
		rs = spr->GetShader().Multiply(filter, rp.render_blend, rp.render_fast_blend, rp.render_downsample);
//		rc = spr->GetCamera() * rp.camera;
		if (rp.actor) {
			rs = rp.actor->GetShader() * rs;
//			rc = rp.actor->GetCamera() * rc;
		}
	} else {
		rs = spr->GetShader().Multiply(filter, rp.render_blend, rp.render_fast_blend, rp.render_downsample);
//		rc = spr->GetCamera() * rp.camera;
	}

	ur::RenderContext* rctx = shader_mgr->GetContext();
	switch (rs.GetFastBlend())
	{
	case s2::FBM_NULL:
		rctx->SetBlend(2, 6);		// BLEND_GL_ONE, BLEND_GL_ONE_MINUS_SRC_ALPHA
		rctx->SetBlendEquation(0);	// BLEND_FUNC_ADD
		break;
	case s2::FBM_ADD:
		rctx->SetBlend(2, 2);		// BLEND_GL_ONE, BLEND_GL_ONE
		rctx->SetBlendEquation(0);	// BLEND_FUNC_ADD
		break;
	case s2::FBM_SUBTRACT:
		rctx->SetBlend(2, 6);		// BLEND_GL_ONE, BLEND_GL_ONE_MINUS_SRC_ALPHA
		rctx->SetBlendEquation(1);	// BLEND_FUNC_SUBTRACT
		break;
	}

	s2::BlendMode blend = s2::BM_NULL;
	if (!rp.IsDisableBlend()) {
		blend = rs.GetBlend();
	}

	filter = s2::FM_NULL;
	if (!rp.IsDisableFilter()) {
		filter = rs.GetFilter();
	}

	if (blend != s2::BM_NULL)
	{
		// todo: DrawBlend()
	}
	else if (filter != s2::FM_NULL)
	{
//		rp.camera = rc;

		if (rp.IsChangeShader()) {
			shader_mgr->SetShader(sl::FILTER);
		}
		sl::FilterShader* shader = static_cast<sl::FilterShader*>(shader_mgr->GetShader(sl::FILTER));
		shader->SetMode(sl::FILTER_MODE(filter));
	}
	else
	{
//		rp.camera = rc;

		if (rp.IsChangeShader()) {
			shader_mgr->SetShader(sl::SPRITE2);
		}
	}
}

}