#ifndef _FLATTEN_COUNT_NODES_VISITOR_H_
#define _FLATTEN_COUNT_NODES_VISITOR_H_

#include <sprite2/SpriteVisitor.h>

namespace ft
{

class CountNodesVisitor : public s2::SpriteVisitor
{
public:
	CountNodesVisitor() : m_count(0) {}

	virtual s2::VisitResult Visit(const s2::Sprite* spr, const s2::SprVisitorParams& params) {
		++m_count;
		return s2::VISIT_INTO;
	}

	int GetCount() const { return m_count; }

private:
	int m_count;

}; // CountNodesVisitor

}

#endif // _FLATTEN_COUNT_NODES_VISITOR_H_