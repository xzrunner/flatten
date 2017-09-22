#ifndef _FLATTEN_LIST_H_
#define _FLATTEN_LIST_H_

#include <cooking/DisplayList.h>

#include <vector>
#include <memory>

#include <stdint.h>

namespace s2 { class Sprite; class Actor; class RenderParams; }

namespace ft
{

class FTNode;

class FTList : public std::enable_shared_from_this<FTList>
{
public:
	FTList(s2::Actor* root);
	FTList(const FTList&) = delete;
	FTList(FTList&&) = delete;
	~FTList();

	bool Update(int pos, bool force, std::shared_ptr<cooking::DisplayList> dlist);
	void DrawForward(int pos, const s2::RenderParams& rp);
	void DrawDeferred(int pos, const s2::RenderParams& rp,
		std::shared_ptr<cooking::DisplayList>& dlist);

	void SetFrame(int pos, bool force, int frame,
		std::shared_ptr<cooking::DisplayList> dlist);

	void SetDirty() { m_dirty = true; }

private:
	void Build(std::shared_ptr<cooking::DisplayList> dlist);

	void InitNeedUpdateFlag();

	bool CheckFirst(int pos, std::shared_ptr<cooking::DisplayList> dlist);

private:
	static const int MAX_LAYER = 16;

private:
	s2::Actor* m_root;

	FTNode* m_nodes;
	int m_nodes_sz, m_nodes_cap;

	int m_max_layer;

	bool m_dirty;

	friend class BuildListVisitor;

}; // FTList

}

#endif // _FLATTEN_FLATTEN_H_
