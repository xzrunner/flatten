#include "flatten/BuildListVisitor.h"
#include "flatten/FTList.h"
#include "flatten/FTNode.h"

#include <sprite2/SprVisitorParams.h>
#include <sprite2/S2_Actor.h>

#include <assert.h>

namespace ft
{

BuildListVisitor::BuildListVisitor(const std::shared_ptr<FTList>& flatten,
	                               const std::shared_ptr<cooking::DisplayList>& dlist)
	: m_flatten(flatten)
	, m_dlist(dlist)
{
}

s2::VisitResult BuildListVisitor::Visit(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	assert(m_flatten->m_nodes_cap > 0 && m_flatten->m_nodes_sz < m_flatten->m_nodes_cap);
	if (params.actor) 
	{
		s2::Actor* actor = const_cast<s2::Actor*>(params.actor);

		int pos = m_flatten->m_nodes_sz;
		m_flatten->m_nodes[m_flatten->m_nodes_sz++].Init(params.actor);
		actor->SetFlatten(m_flatten, pos);

		if (m_dlist) {
			actor->SetDisplayList(m_dlist);
		}
	} 
	else 
	{
		m_flatten->m_nodes[m_flatten->m_nodes_sz++].Init(spr);
	}
	FTNode& node = m_flatten->m_nodes[m_flatten->m_nodes_sz - 1];
	if (m_curr_path.empty()) {
		node.m_parent = -1;
	} else {
		node.m_parent = m_curr_path.back();
	}
	return s2::VISIT_INTO;
}

s2::VisitResult BuildListVisitor::VisitChildrenBegin(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	m_curr_path.push_back(m_flatten->m_nodes_sz - 1);
	return s2::VISIT_OVER;
}

s2::VisitResult BuildListVisitor::VisitChildrenEnd(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	int curr = m_curr_path.back();
	m_curr_path.pop_back();
	int tot = m_flatten->m_nodes_sz;
	assert(tot >= curr);
	FTNode& node = m_flatten->m_nodes[curr];
	node.m_count = tot - curr;
	node.m_layer = static_cast<uint16_t>(m_curr_path.size());
	if (node.m_layer > m_flatten->m_max_layer) {
		m_flatten->m_max_layer = node.m_layer;
	}
	return s2::VISIT_OVER;
}

}