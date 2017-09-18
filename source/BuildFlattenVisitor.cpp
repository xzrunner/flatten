#include "flatten/BuildFlattenVisitor.h"
#include "flatten/Flatten.h"

#include <sprite2/SprVisitorParams.h>

#include <assert.h>

namespace ft
{

BuildFlattenVisitor::BuildFlattenVisitor(Flatten& flatten)
	: m_flatten(flatten)
{
}

s2::VisitResult BuildFlattenVisitor::Visit(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	if (params.actor) {
		m_flatten.m_nodes.push_back(Flatten::Node(params.actor));
	} else {
		m_flatten.m_nodes.push_back(Flatten::Node(spr));
	}
	Flatten::Node& node = m_flatten.m_nodes.back();
	if (m_curr_path.empty()) {
		node.m_parent = -1;
	} else {
		node.m_parent = m_curr_path.back();
	}
	return s2::VISIT_INTO;
}

s2::VisitResult BuildFlattenVisitor::VisitChildrenBegin(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	m_curr_path.push_back(m_flatten.m_nodes.size() - 1);
	return s2::VISIT_OVER;
}

s2::VisitResult BuildFlattenVisitor::VisitChildrenEnd(const s2::Sprite* spr, const s2::SprVisitorParams& params)
{
	int curr = m_curr_path.back();
	m_curr_path.pop_back();
	int tot = m_flatten.m_nodes.size();
	assert(tot >= curr);
	Flatten::Node& node = m_flatten.m_nodes[curr];
	node.m_count = tot - curr;
	node.m_layer = m_curr_path.size();
	return s2::VISIT_OVER;
}

}