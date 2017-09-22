#ifndef _FLATTEN_LIST_H_
#define _FLATTEN_LIST_H_

#include <cooking/DisplayList.h>

#include <vector>
#include <memory>

#include <stdint.h>

namespace s2 { class Sprite; class Actor; class RenderParams; }

namespace ft
{

class Node;

class List : public std::enable_shared_from_this<List>
{
public:
	List(s2::Actor* root);
	List(const List&) = delete;
	List(List&&) = delete;
	~List();

	bool Update(int pos, bool force);
	void DrawForward(int pos, const s2::RenderParams& rp);
	void DrawDeferred(int pos, const s2::RenderParams& rp,
		std::unique_ptr<cooking::DisplayList>& dlist);

	void SetFrame(int pos, bool force, int frame);

	void SetDirty() { m_dirty = true; }

private:
	void Build();

	void InitNeedUpdateFlag();

	bool CheckFirst(int pos);

private:
	static const int MAX_LAYER = 16;

private:
	s2::Actor* m_root;

	Node* m_nodes;
	int m_nodes_sz, m_nodes_cap;

	int m_max_layer;

	bool m_dirty;

	friend class BuildListVisitor;

}; // List

}

#endif // _FLATTEN_FLATTEN_H_
