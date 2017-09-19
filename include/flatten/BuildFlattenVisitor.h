#ifndef _FLATTEN_BUILD_FLATTEN_VISITOR_H_
#define _FLATTEN_BUILD_FLATTEN_VISITOR_H_

#include <sprite2/s2_config.h>
#include <sprite2/SpriteVisitor.h>

#include <vector>
#include <memory>

namespace ft
{

class Flatten;

class BuildFlattenVisitor : public s2::SpriteVisitor
{
public:
	BuildFlattenVisitor(std::shared_ptr<ft::Flatten> flatten);

	virtual s2::VisitResult Visit(const s2::Sprite* spr, const s2::SprVisitorParams& params);

	virtual s2::VisitResult VisitChildrenBegin(const s2::Sprite* spr, const s2::SprVisitorParams& params);
	virtual s2::VisitResult VisitChildrenEnd(const s2::Sprite* spr, const s2::SprVisitorParams& params);

private:
	std::shared_ptr<ft::Flatten> m_flatten;

	std::vector<int> m_curr_path;

}; // BuildFlattenVisitor

}

#endif // _FLATTEN_BUILD_FLATTEN_VISITOR_H_