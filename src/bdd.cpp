// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <cassert>
#include <algorithm>
#include "bdd.h"
#include "output.h"
using namespace std;

#define MEMO

template<typename T> struct veccmp {
	bool operator()(const vector<T>& x, const vector<T>& y) const{
		if (x.size() != y.size()) return x.size() < y.size();
		return x < y;
	}
};

template<typename T1, typename T2> struct vec2cmp {
	typedef pair<std::vector<T1>, vector<T2>> t;
	bool operator()(const t& x, const t& y) const {
		static veccmp<T1> t1;
		static veccmp<T2> t2;
		if (x.first != y.first) return t1(x.first, y.first);
		return t2(x.second, y.second);
	}
};

vector<unordered_map<bdd_key, int_t>> Mp, Mn;
vector<bdd> V;
unordered_map<ite_memo, int_t> C;
map<bools, unordered_map<array<int_t, 2>, int_t>, veccmp<bool>> CX;
map<pair<bools, uints>, unordered_map<array<int_t, 2>, int_t>,
	vec2cmp<bool, uint_t>> CXP;
unordered_map<bdds, int_t> AM;
map<bools, unordered_map<bdds, int_t>, veccmp<bool>> AMX;
map<pair<bools, uints>, unordered_map<bdds, int_t>, vec2cmp<bool, uint_t>> AMXP;
unordered_set<int_t> S;
unordered_map<int_t, weak_ptr<bdd_handle>> bdd_handle::M;
spbdd_handle bdd_handle::T, bdd_handle::F;
map<bools, unordered_map<int_t, int_t>, veccmp<bool>> memos_ex;
map<uints, unordered_map<int_t, int_t>, veccmp<uint_t>> memos_perm;
map<pair<uints, bools>, unordered_map<int_t, int_t>, vec2cmp<uint_t, bool>>
	memos_perm_ex;

auto am_cmp = [](int_t x, int_t y) {
	bool s = x < y;
	x = abs(x), y = abs(y);
	return x < y ? true : x == y ? s : false;
};

const size_t gclimit = 1e+6;

void bdd::init() {
	S.insert(0), S.insert(1), V.emplace_back(0, 0, 0), // dummy
	V.emplace_back(0, 1, 1), Mp.resize(1),
	Mp[0].emplace(bdd_key(hash_pair(0, 0), 0, 0), 0),
	Mp[0].emplace(bdd_key(hash_pair(1, 1), 1, 1), 1),
	bdd_handle::T = bdd_handle::get(T), bdd_handle::F = bdd_handle::get(F);
}

//int_t bdd::add(int_t v, int_t h, int_t l) {
//	DBG(assert(h && l && v > 0);)
//	DBG(assert(leaf(h) || v < abs(V[abs(h)].v));)
//	DBG(assert(leaf(l) || v < abs(V[abs(l)].v));)
//	if (h == l) return h;
//	if (abs(h) < abs(l)) swap(h, l), v = -v;
//	static std::unordered_map<bdd_key, int_t>::const_iterator it;
//	static bdd_key k;
//	auto &mm = v < 0 ? Mn : Mp;
//	if (mm.size() <= (size_t)abs(v)) mm.resize(abs(v)+1);
//	auto &m = mm[abs(v)];
//	if (l < 0) {
//		k = bdd_key(hash_pair(-h, -l), -h, -l);
//		return	(it = m.find(k)) != m.end() ? -it->second :
//			(V.emplace_back(v, -h, -l),
//			m.emplace(std::move(k), V.size()-1),
//			-V.size()+1);
//	}
//	k = bdd_key(hash_pair(h, l), h, l);
//	return	(it = m.find(k)) != m.end() ? it->second :
//		(V.emplace_back(v, h, l),
//		m.emplace(std::move(k), V.size()-1),
//		V.size()-1);
//}

int_t bdd::from_bit(uint_t b, bool v) {
	return v ? add(b + 1, T, F) : add(b + 1, F, T);
}

bool bdd::bdd_subsumes(int_t x, int_t y) {
	if (x == T) return true;
	if (x == F) return y == F;
	if (y == T) return false;
	if (y == F) return true;
	const bdd bx = get(x), by = get(y);
	if (bx.v < by.v) return bdd_subsumes(bx.h, y) && bdd_subsumes(bx.l, y);
	if (bx.v > by.v) return bdd_subsumes(x, by.h) && bdd_subsumes(x, by.l);
	return bdd_subsumes(bx.h, by.h) && bdd_subsumes(bx.l, by.l);
}

int stackdepth = 0, maxstackdepth = 0;
clock_t total = 0;
int countand = 0;

int_t bdd::bdd_and(int_t x, int_t y) {
	//clock_t start = clock();

	//auto retval = bdd_and_recursive(x, y);
	//auto retval = and_flat(x, y);
	auto retval = and_stack(x, y);

	//clock_t end = clock();
	//total += double(end - start);
	//countand++;
	//if (true) { //if ((countand % 100) == 0) { //countand == 10000) {
	//	output::to(L"info") << "test and(node): (" << x << ", " << y << ") =>: " << retval << endl;
	//	output::to(L"info") << "test and: " << double(end - start) << " (" << total << "), ret: " << retval << endl; // / CLOCKS_PER_SEC
	//}
	//if (stackdepth > maxstackdepth) {
	//	output::to(L"info") << "depth(stack): (" << maxstackdepth << ") =>: " << stackdepth << endl;
	//	maxstackdepth = stackdepth;
	//}
	return retval;
}
int_t bdd::bdd_and_recursive(int_t x, int_t y) {
	DBG(assert(x && y);)
	if (x == F || y == F || x == -y) return F;
	if (x == T || x == y) return y;
	if (y == T) return x;
	if (x > y) swap(x, y);
#ifdef MEMO
	if (C.size() >= gclimit) {
		const bdd bx = get(x), by = get(y);
		if (bx.v < by.v)
			return add(bx.v, bdd_and_recursive(bx.h, y), bdd_and_recursive(bx.l, y));
		else if (bx.v > by.v)
			return add(by.v, bdd_and_recursive(x, by.h), bdd_and_recursive(x, by.l));
		return add(bx.v, bdd_and_recursive(bx.h, by.h), bdd_and_recursive(bx.l, by.l));
	}
	ite_memo m = { x, y, F };
	auto it = C.find(m);
	if (it != C.end()) return it->second;
#endif
	const bdd bx = get(x), by = get(y);
	int_t r;
	if (bx.v < by.v) r = add(bx.v, bdd_and_recursive(bx.h, y), bdd_and_recursive(bx.l, y));
	else if (bx.v > by.v) r = add(by.v, bdd_and_recursive(x, by.h), bdd_and_recursive(x, by.l));
	else r = add(bx.v, bdd_and_recursive(bx.h, by.h), bdd_and_recursive(bx.l, by.l));
#ifdef MEMO
	C.emplace(m, r);
#endif
	return r;
}

vector<xyitem> stack(100); // (200); // 20000);
ullong fcache[10000];

int_t bdd::and_stack(int_t x, int_t y) {
	DBG(assert(x && y););
	DBG(assert(int(stack.size() > 0)););

	int ntries = 0;
	int ipop = -1;
	int_t value;
	//constexpr int imax = std::numeric_limits<int>::max();

	xynode node = { x, y };
	//const bool hasspace = int(stack.size()) - 1 > ipop;
	stack[++ipop] = { node, StackState::Tail }; // set directly instead

	while (ipop >= 0) { //!stack.empty()) {
		xyitem& stackinfo = stack[ipop--];

		xynode& node = stackinfo.node;
		StackState state = stackinfo.state;
		int_t& x = node.x;
		int_t& y = node.y;

		switch (state) {
			case StackState::Tail:
			{
				bool istail = node.prod < std::numeric_limits<int>::max();
				if (istail) {
					value = node.prod;
					break;
				}

				// we're not tail, get left/right nodes and dive deeper...
				//auto fork = getfork(x, y);
				std::array<int_t, 5> forkarr; // [5] ;
				const bdd bx = get(x), by = get(y);
				if (bx.v < by.v) forkarr = { bx.v, bx.h, y, bx.l, y };
				else if (bx.v > by.v) forkarr = { by.v, x, by.h, x, by.l };
				else forkarr = { bx.v, bx.h, by.h, bx.l, by.l };

				xyitem& item = stack[++ipop];
				//item.node = stackinfo.node;
				item.node.x = stackinfo.node.x;
				item.node.y = stackinfo.node.y;
				item.node.prod = stackinfo.node.prod;
				//item.right = fork.right;
				item.right.x = forkarr[3];
				item.right.y = forkarr[4];
				item.right.calctail();
				item.val = forkarr[0]; // fork.val;
				item.state = StackState::Left;

				const bool hasspace = int(stack.size()) - 1 > ipop;
				if (hasspace) {
					xyitem& item = stack[++ipop];
					//item.node = fork.left;
					item.node.x = forkarr[1];
					item.node.y = forkarr[2];
					item.node.calctail();
					item.state = StackState::Tail;
				}
				else { // this shouldn't happen (increase stack size if needed)
					if ((ntries++ % 100) == 0) {
						output::to(L"info") << "stack.emplace_back: (" << stack.size() << ", " << ipop << ") =>: " << ntries << endl;
						output::to(L"info") << "stack.emplace_back: (" << x << ", " << y << ") =>: " << ntries << endl;
					}
					stack.emplace_back(xynode(forkarr[1], forkarr[2]), StackState::Tail);
					ipop++;
				}
				break;
			}
			case StackState::Left:
			{
				xyitem& item = stack[++ipop];
				//item.node = stackinfo.node;
				item.node.x = stackinfo.node.x;
				item.node.y = stackinfo.node.y;
				item.node.prod = stackinfo.node.prod;
				item.val = stackinfo.val;
				item.leftval = value;
				item.state = StackState::Right;

				const bool hasspace = int(stack.size()) - 1 > ipop;
				if (hasspace) {
					xyitem& item = stack[++ipop];
					//item.node = stackinfo.right;
					item.node.x = stackinfo.right.x;
					item.node.y = stackinfo.right.y;
					item.node.prod = stackinfo.right.prod;
					item.state = StackState::Tail;
				}
				else { // shouldn't happen (increase stack preallocated size)
					if ((ntries++ % 100) == 0) {
						output::to(L"info") << "stack.emplace_back(r): (" << stack.size() << ", " << ipop << ") =>: " << ntries << endl;
						output::to(L"info") << "stack.emplace_back(r): (" << x << ", " << y << ") =>: " << ntries << endl;
					}
					stack.emplace_back(stackinfo.right, StackState::Tail);
					ipop++;
				}
				break;
			}
			case StackState::Right:
			{
				stackinfo.rightval = value;
				ite_memo m = { x, y, F };
				//auto it = C.find(m); if (it != C.end()) {} // assert or something
				int_t r = add(stackinfo.val, stackinfo.leftval, stackinfo.rightval);
				C.emplace(m, r);
				uint_t shash = hash_pair(x, y); // x + y;
				uint_t hash = shash % 640000; // hash => 
				int index = hash / 64; // 0..999
				int bit = hash - index * 64; // 0..63
				fcache[index] |= (1u << bit);
				value = r;
				stackinfo.result = value; // in case we decide to do this via stack items
				break;
			}
			default:
				throw 0;
		}
	}
	return value; // *value;
}

//int_t bdd::and_flat(int_t x, int_t y) {
//	xynode node = { x, y };
//	return and_flat(node);
//	// we don't care about the return here, return value is in retval instead.
//	// also we don't care about any intermediate bdd_and returns. 
//	// 'C' is also up-to-date so we could get the value from it as well (any node).
//}
//
//// how this works: left-s go first, we go from left tail up, left tail sets
//// the first leftval for its parent. Parent then pops and forks, left is
//// done (we have leftval just set), parent adds itself to the 'right-tail'
//// unwind-stack, and adds the right child (& its left-s) to the left stack.
//// we continue left stack from now newly added left tail and work our way up
//// note: if there's only one left, leftval is set as its tail and returns.
//// if more, leftest tail sets the first leftval, goes up, saves to unwind,
//// goes right and as soon as we hit the right-tail stack unwinds up to the
//// top-left node. 'retval' is set to its 'add' result, which is the new
//// leftval for its parent.
//int_t bdd::and_flat(xynode& node) {
//	DBG(assert(node.x && node.y););
//
//	vector<addnode> parents = {};
//	int_t retval;
//
//	// 'parents' holds the parent node 'add' stuff, 'node' is its right fork
//	vector<xystackitem> leftparents = {};
//	optional<int_t> leftval; // first leftval is of no importance
//	leftparents.emplace_back(node, parents);
//	auto leftnode = node;
//	while (!istail(leftnode)) {
//		leftnode = getfork(leftnode).left;
//		// since these are all left nodes, parents don't apply and we reset to {}
//		// ...unless we're the last node left (i.e. the 'node')
//		leftparents.emplace_back(leftnode, vector<addnode>());
//	}
//	// first node to pop will always be a 'tail' (guaranteed by the while above)
//	// (we also always have l/r/fork, i.e. unless the tail condition is triggered)
//	// (if 'node' is a tail itself it'll just fall through and do one-pass here)
//	while (!leftparents.empty()) {
//		auto stackinfo = leftparents.back();
//		leftparents.pop_back();
//
//		xynode& node = stackinfo.node;
//		vector<addnode>& rightparents = stackinfo.parents;
//		int_t& x = node.x;
//		int_t& y = node.y;
//
//		if (auto value = baseexit(x, y, rightparents, retval)) {
//			// retval is the real return (on tail exit), this being the top node.
//			// when no parents retval will be set to *value (for left node)
//			// when we have right node w/ parents, we don't care about intermediates
//			leftval = retval; // *value;
//			continue;
//		}
//
//		// we're not tail, get left/right nodes and dive deeper...
//		auto fork = getfork(x, y);
//
//		// this is for 'add' to unwind once the right-most node hits the tail.
//		// this was essential to get a sort of tail-right-recursion to be able to
//		// merge the right recursion (and get 2 similar while loops into one).
//		rightparents.emplace_back(x, y, fork.val, *leftval); // leftval is always ok
//
//		// add right's child nodes to the stack, it'll work nicely (this is the key)
//		leftparents.emplace_back(fork.right, rightparents);
//		auto leftnode = fork.right;
//		while (!istail(leftnode)) {
//			leftnode = getfork(leftnode).left;
//			leftparents.emplace_back(leftnode, vector<addnode>());
//		}
//	}
//	return *leftval; // retval // the same
//}

int_t bdd::bdd_ite_var(uint_t x, int_t y, int_t z) {
	if (x+1 < var(y) && x+1 < var(z)) return add(x+1, y, z);
	return bdd_ite(from_bit(x, true), y, z);
}

int_t bdd::bdd_ite(int_t x, int_t y, int_t z) {
	DBG(assert(x && y && z);)
	if (x < 0) return bdd_ite(-x, z, y);
	if (x == F) return z;
	if (x == T || y == z) return y;
	if (x == -y || x == z) return F;
	if (y == T) return bdd_or(x, z);
	if (y == F) return bdd_and(-x, z);
	if (z == F) return bdd_and(x, y);
	if (z == T) return bdd_or(-x, y);
	auto it = C.find({x, y, z});
	if (it != C.end()) return it->second;
	int_t r;
	const bdd bx = get(x), by = get(y), bz = get(z);
	const int_t s = min(bx.v, min(by.v, bz.v));
	if (bx.v == by.v && by.v == bz.v)
		r =	add(bx.v, bdd_ite(bx.h, by.h, bz.h),
				bdd_ite(bx.l, by.l, bz.l));
	else if (s == bx.v && s == by.v)
		r =	add(bx.v, bdd_ite(bx.h, by.h, z),
				bdd_ite(bx.l, by.l, z));
	else if (s == by.v && s == bz.v)
		r =	add(by.v, bdd_ite(x, by.h, bz.h),
				bdd_ite(x, by.l, bz.l));
	else if (s == bx.v && s == bz.v)
		r =	add(bx.v, bdd_ite(bx.h, y, bz.h),
				bdd_ite(bx.l, y, bz.l));
	else if (s == bx.v)
		r =	add(bx.v, bdd_ite(bx.h, y, z), bdd_ite(bx.l, y, z));
	else if (s == by.v)
		r =	add(by.v, bdd_ite(x, by.h, z), bdd_ite(x, by.l, z));
	else	r =	add(bz.v, bdd_ite(x, y, bz.h), bdd_ite(x, y, bz.l));
	if (C.size() > gclimit) return r;
	return C.emplace(ite_memo{x, y, z}, r), r;
}

void am_sort(bdds& b) {
	sort(b.begin(), b.end(), am_cmp);
	for (size_t n = 0; n < b.size();)
		if (b[n] == T) b.erase(b.begin() + n);
		else if (b[n] == F) { b = {F}; return; }
		else if (!n) { ++n; continue; }
		else if (b[n] == b[n-1]) b.erase(b.begin() + n);
		else if (b[n] == -b[n-1]) { b = {F}; return; }
		else ++n;
}

size_t bdd::bdd_and_many_iter(bdds v, bdds& h, bdds& l, int_t &res, size_t &m){
	size_t i;
	bool flag = false;
	m = var(v[0]);
	for (i = 1; i != v.size(); ++i)
		if (!leaf(v[i])) {
			if (var(v[i]) < m) m = var(v[i]);
		} else if (!trueleaf(v[i])) return res = F, 1;
	h.reserve(v.size()), l.reserve(v.size());
	for (i = 0; i != v.size(); ++i)
		if (var(v[i]) != m) h.push_back(v[i]);
		else if (!leaf(hi(v[i]))) h.push_back(hi(v[i]));
		else if (!trueleaf(hi(v[i]))) { flag = true; break; }
	if (!flag) am_sort(h);
	for (i = 0; i != v.size(); ++i)
		if (var(v[i]) != m) l.push_back(v[i]);
		else if (!leaf(lo(v[i]))) l.push_back(lo(v[i]));
		else if (!trueleaf(lo(v[i]))) return flag ? res = F, 1 : 2;
	am_sort(l);
	if (!flag) { if (h.size() && h[0] == F) flag = true; }
	if (l.size() && l[0] == F) return flag ? 3 : 2;
	if (flag) return 3;
	bdds x;
	set_intersection(h.begin(),h.end(),l.begin(),l.end(),back_inserter(x),
			am_cmp);
	am_sort(x);
	if (x.size() > 1) {
		for (size_t n = 0; n < h.size();)
			if (hasbc(x, h[n], am_cmp)) h.erase(h.begin() + n);
			else ++n;
		for (size_t n = 0; n < l.size();)
			if (hasbc(x, l[n], am_cmp)) l.erase(l.begin() + n);
			else ++n;
		h.shrink_to_fit(), l.shrink_to_fit(), x.shrink_to_fit();
		int_t r = bdd_and_many(move(x));
		if (r == F) return res = F, 1;
		if (r != T) {
			if (!hasbc(h, r, am_cmp)) h.push_back(r), am_sort(h);
			if (!hasbc(l, r, am_cmp)) l.push_back(r), am_sort(l);
		}
	}
	return 0;
}

bool subset(const bdds& small, const bdds& big) {
	if (	big.size() < small.size() ||
		am_cmp(abs(big[big.size()-1]), abs(small[0])) ||
		am_cmp(abs(small[small.size()-1]), abs(big[0])))
		return false;
	for (const int_t& t : small) if (!hasbc(big, t, am_cmp)) return false;
	return true;
}

size_t simps = 0;

bool bdd::am_simplify(bdds& v, const unordered_map<bdds, int_t>& memo) {
	for (auto x : memo)
		if (subset(x.first, v)) {
			if (x.second == F) return v={F}, true;
			for (size_t n = 0; n < v.size();)
				if (!hasbc(x.first, v[n], am_cmp)) ++n;
				else v.erase(v.begin() + n);
			if (!hasbc(v, x.second, am_cmp)) v.push_back(x.second);
			return true;
		}
	return false;
}

int_t bdd::bdd_and_many(bdds v) {
#ifdef MEMO
	static unordered_map<ite_memo, int_t>::const_iterator jt;
	for (size_t n = 0; n < v.size(); ++n)
		for (size_t k = 0; k < n; ++k) {
			int_t x, y;
			if (v[n] < v[k]) x = v[n], y = v[k];
			else x = v[k], y = v[n];
			if ((jt = C.find({x, y, F})) != C.end()) {
				v.erase(v.begin()+k), v.erase(v.begin()+n-1),
				v.push_back(jt->second), n = k = 0;
				break;
			}
		}
#endif
	if (v.empty()) return T;
	if (v.size() == 1) return v[0];
	simps = 0;
	static bdds v1;
	do {
		if (v1=v, am_simplify(v, AM), ++simps, v.size()==1) return v[0];
	} while (v1 != v);
	if (v.empty()) return T;
	if (v.size() == 1) return v[0];
	auto it = AM.find(v);
	if (it != AM.end()) return it->second;
	if (v.size() == 2)
		return AM.emplace(v, bdd_and(v[0], v[1])).first->second;
	int_t res = F, h, l;
	size_t m = 0;
	bdds vh, vl;
	switch (bdd_and_many_iter(v, vh, vl, res, m)) {
		case 0: l = bdd_and_many(move(vl)),
			h = bdd_and_many(move(vh));
			break;
		case 1: return AM.emplace(v, res), res;
		case 2: h = bdd_and_many(move(vh)), l = F; break;
		case 3: h = F, l = bdd_and_many(move(vl)); break;
		default: throw 0;
	}
	return AM.emplace(v, bdd::add(m, h, l)).first->second;
}

int_t bdd::bdd_and_ex(int_t x, int_t y, const bools& ex,
	unordered_map<array<int_t, 2>, int_t>& memo,
	unordered_map<int_t, int_t>& m2, int_t last) {
	DBG(assert(x && y);)
	if (x == F || y == F || x == -y) return F;
	if (x == T || x == y) return bdd_ex(y, ex, m2, last);
	if (y == T) return bdd_ex(x, ex, m2, last);
	if (x > y) swap(x, y);
	array<int_t, 2> m = {x, y};
	auto it = memo.find(m);
	if (it != memo.end()) return it->second;
	const bdd bx = get(x), by = get(y);
	int_t rx, ry, v, r;
	if (bx.v > last+1 && by.v > last+1)
		return memo.emplace(m, r = bdd_and(x, y)), r;
	if (bx.v < by.v)
		v = bx.v,
		rx = bdd_and_ex(bx.h, y, ex, memo, m2, last),
		ry = bdd_and_ex(bx.l, y, ex, memo, m2, last);
	else if (bx.v > by.v)
		v = by.v,
		rx = bdd_and_ex(x, by.h, ex, memo, m2, last),
		ry = bdd_and_ex(x, by.l, ex, memo, m2, last);
	else
		v = bx.v,
		rx = bdd_and_ex(bx.h, by.h, ex, memo, m2, last),
		ry = bdd_and_ex(bx.l, by.l, ex, memo, m2, last);
	DBG(assert((size_t)v - 1 < ex.size());)
	r = ex[v - 1] ? bdd_or(rx, ry) : add(v, rx, ry);
	return memo.emplace(m, r), r;
}

struct sbdd_and_ex_perm {
	const bools& ex;
	const uints& p;
	unordered_map<array<int_t, 2>, int_t>& memo;
	unordered_map<int_t, int_t>& m2;
	int_t last;

	sbdd_and_ex_perm(const bools& ex, const uints& p,
	unordered_map<array<int_t, 2>, int_t>& memo,
	unordered_map<int_t, int_t>& m2) :
		ex(ex), p(p), memo(memo), m2(m2), last(0) {
			for (size_t n = 0; n != ex.size(); ++n)
				if (ex[n] || (p[n] != n)) last = n;
		}

	int_t operator()(int_t x, int_t y) {
		DBG(assert(x && y);)
		if (x == F || y == F || x == -y) return F;
		if (x == T || x == y)
			return bdd::bdd_permute_ex(y, ex, p, last, m2);
		if (y == T) return bdd::bdd_permute_ex(x, ex, p, last, m2);
		if (x > y) swap(x, y);
		array<int_t, 2> m = {x, y};
		auto it = memo.find(m);
		if (it != memo.end()) return it->second;
		const bdd bx = bdd::get(x), by = bdd::get(y);
		int_t rx, ry, v, r;
		if (bx.v > last+1 && by.v > last+1)
			return memo.emplace(m, r = bdd::bdd_and(x, y)), r;
		if (bx.v < by.v)
			v = bx.v, rx = (*this)(bx.h, y), ry = (*this)(bx.l, y);
		else if (bx.v > by.v)
			v = by.v, rx = (*this)(x, by.h), ry = (*this)(x, by.l);
		else v = bx.v, rx = (*this)(bx.h,by.h), ry = (*this)(bx.l,by.l);
		DBG(assert((size_t)v - 1 < ex.size());)
		r = ex[v - 1] ? bdd::bdd_or(rx, ry) :
			bdd::bdd_ite_var(p[v-1], rx, ry);
		return memo.emplace(m, r), r;
	}
};

int_t bdd::bdd_and_ex(int_t x, int_t y, const bools& ex) {
	int_t last = 0;
	for (size_t n = 0; n != ex.size(); ++n) if (ex[n]) last = n;
	int_t r = bdd_and_ex(x, y, ex, CX[ex], memos_ex[ex], last);
	DBG(int_t t = bdd_ex(bdd_and(x, y), ex);)
	DBG(assert(r == t);)
	return r;
}

int_t bdd::bdd_and_ex_perm(int_t x, int_t y, const bools& ex, const uints& p) {
	return sbdd_and_ex_perm(ex,p,CXP[{ex,p}],memos_perm_ex[{p,ex}])(x,y);
}

char bdd::bdd_and_many_ex_iter(const bdds& v, bdds& h, bdds& l, int_t& m) {
	size_t i, sh = 0, sl = 0;
	bdd *b = (bdd*)alloca(sizeof(bdd) * v.size());
	for (i = 0; i != v.size(); ++i) b[i] = get(v[i]);
	m = b[0].v;//var(v[0]);
	for (i = 1; i != v.size(); ++i) m = min(m, b[i].v);//var(v[i]));
	int_t *ph = (int_t*)alloca(sizeof(int_t) * v.size()),
	      *pl = (int_t*)alloca(sizeof(int_t) * v.size());
	for (i = 0; ph && i != v.size(); ++i)
		if (b[i].v != m) ph[sh++] = v[i];
		else if (b[i].h == F) ph = 0;
		else if (b[i].h != T) ph[sh++] = b[i].h;
	for (i = 0; pl && i != v.size(); ++i)
		if (b[i].v != m) pl[sl++] = v[i];
		else if (b[i].l == F) pl = 0;
		else if (b[i].l != T) pl[sl++] = b[i].l;
	if (ph) {
		h.resize(sh);
		while (sh--) h[sh] = ph[sh];
		am_sort(h);
	}
	if (pl) {
		l.resize(sl);
		while (sl--) l[sl] = pl[sl];
		am_sort(l);
		return ph ? 0 : 2;
	}
	return ph ? 1 : 3;
}

struct sbdd_and_many_ex {
	const bools& ex;
	unordered_map<bdds, int_t>& memo;
	unordered_map<int_t, int_t>& m2;
	unordered_map<array<int_t, 2>, int_t>& m3;
	int_t last;

	sbdd_and_many_ex(const bools& ex, unordered_map<bdds, int_t>& memo,
		unordered_map<int_t, int_t>& m2,
		unordered_map<array<int_t, 2>, int_t>& m3) :
		ex(ex), memo(memo), m2(m2), m3(m3), last(0) {
		for (size_t n = 0; n != ex.size(); ++n) if (ex[n]) last = n;
	}

	int_t operator()(bdds v) {
		if (v.empty()) return T;
		if (v.size() == 1) return bdd::bdd_ex(v[0], ex, m2, last);
		if (v.size() == 2)
			return bdd::bdd_and_ex(v[0], v[1], ex, m3, m2, last);
		auto it = memo.find(v);
		if (it != memo.end()) return it->second;
		int_t res = F, h, l, m = 0;
		bdds vh, vl;
		char c = bdd::bdd_and_many_ex_iter(v, vh, vl, m);
		if (m > last+1) {
			switch (c) {
			case 0: res = bdd::add(m, bdd::bdd_and_many(move(vh)),
				bdd::bdd_and_many(move(vl))); break;
			case 1: res = bdd::add(m,bdd::bdd_and_many(move(vh)),F);
				break;
			case 2: res = bdd::add(m,F,bdd::bdd_and_many(move(vl)));
				break;
			case 3: res = F; break;
			default: throw 0;
			}
		} else {
			switch (c) {
			case 0: l = (*this)(move(vl)), h = (*this)(move(vh));
				if (ex[m - 1]) res = bdd::bdd_or(h, l);
				else res = bdd::add(m, h, l);
				break;
			case 1: if (ex[m - 1]) res = (*this)(move(vh));
				else res = bdd::add(m, (*this)(move(vh)), F);
				break;
			case 2: if (ex[m - 1]) res = (*this)(move(vl));
				else res = bdd::add(m, F, (*this)(move(vl)));
				break;
			case 3: res = F; break;
			default: throw 0;
			}
		}
		return memo.emplace(move(v), res).first->second;
	}
};

struct sbdd_and_many_ex_perm {
	const bools& ex;
	const uints& p;
	unordered_map<bdds, int_t>& memo;
	//map<bdds, int_t, veccmp<int_t>>& memo;
	unordered_map<array<int_t, 2>, int_t>& m2;
	unordered_map<int_t, int_t>& m3;
	int_t last;
	sbdd_and_ex_perm saep;

	sbdd_and_many_ex_perm(const bools& ex, const uints& p,
		unordered_map<bdds, int_t>& memo,
		unordered_map<array<int_t, 2>, int_t>& m2,
		unordered_map<int_t, int_t>& m3) :
		ex(ex), p(p), memo(memo), m2(m2), m3(m3), last(0),
		saep(ex, p, m2, m3) {
			for (size_t n = 0; n != ex.size(); ++n)
				if (ex[n] || (p[n] != n)) last = n;
		}

	int_t operator()(bdds v) {
		if (v.empty()) return T;
		if (v.size() == 1)
			return bdd::bdd_permute_ex(v[0], ex, p, last, m3);
		if (v.size() == 2) return saep(v[0], v[1]);
		auto it = memo.find(v);
		if (it != memo.end()) return it->second;
		int_t res = F, h, l, m = 0;
		bdds vh, vl;
		char c = bdd::bdd_and_many_ex_iter(v, vh, vl, m);
		if (m > last+1) {
			switch (c) {
			case 0: res = bdd::add(m, bdd::bdd_and_many(move(vh)),
					bdd::bdd_and_many(move(vl))); break;
			case 1: res = bdd::add(m,bdd::bdd_and_many(move(vh)),F);
				break;
			case 2: res = bdd::add(m,F,bdd::bdd_and_many(move(vl)));
				break;
			case 3: res = F; break;
			default: throw 0;
			}
		} else {
			switch (c) {
			case 0: l = (*this)(move(vl)), h = (*this)(move(vh));
				if (ex[m - 1]) res = bdd::bdd_or(h, l);
				else res = bdd::bdd_ite_var(p[m-1],h,l);
				break;
			case 1: if (ex[m - 1]) res = (*this)(move(vh));
				else res = bdd::bdd_ite_var(p[m-1],
					(*this)(move(vh)), F);
				break;
			case 2: if (ex[m - 1]) res = (*this)(move(vl));
				else res = bdd::bdd_ite_var(p[m-1], F,
					(*this)(move(vl)));
				break;
			case 3: res = F; break;
			default: throw 0;
			}
		}
		return memo.emplace(move(v), res), res;
	}
};

int_t bdd::bdd_and_many_ex(bdds v, const bools& ex) {
	int_t r;
	DBG(int_t t = bdd_ex(bdd_and_many(v), ex);)
	r = sbdd_and_many_ex(ex, AMX[ex], memos_ex[ex], CX[ex])(v);
	DBG(assert(r == t);)
	return r;
}

int_t bdd::bdd_and_many_ex_perm(bdds v, const bools& ex, const uints& p) {
	return sbdd_and_many_ex_perm(ex, p, AMXP[{ex,p}], CXP[{ex,p}],
			memos_perm_ex[{p,ex}])(v);
}

void bdd::mark_all(int_t i) {
	DBG(assert((size_t)abs(i) < V.size());)
	if ((i = abs(i)) >= 2 && !has(S, i))
		mark_all(hi(i)), mark_all(lo(i)), S.insert(i);
}

void bdd::gc() {
	S.clear();
	for (auto x : bdd_handle::M) mark_all(x.first);
//	if (V.size() < S.size() << 3) return;
	const size_t pvars = Mp.size(), nvars = Mn.size();
	Mp.clear(), Mn.clear(), S.insert(0), S.insert(1);
//	if (S.size() >= 1e+6) { wcerr << "out of memory" << endl; exit(1); }
	vector<int_t> p(V.size(), 0);
	vector<bdd> v1;
	v1.reserve(S.size());
	for (size_t n = 0; n < V.size(); ++n)
		if (has(S, n)) p[n] = v1.size(), v1.emplace_back(move(V[n]));
	output::to(L"error") << "S: " << S.size() << " V: "<< V.size() <<
		" AM: " << AM.size() << " C: "<< C.size() << endl;
	V = move(v1);
#define f(i) (i = (i >= 0 ? p[i] ? p[i] : i : p[-i] ? -p[-i] : i))
	for (size_t n = 2; n < V.size(); ++n) {
		DBG(assert(p[abs(V[n].h)] && p[abs(V[n].l)] && V[n].v);)
		f(V[n].h), f(V[n].l);
	}
	unordered_map<ite_memo, int_t> c;
	unordered_map<bdds, int_t> am;
	for (pair<ite_memo, int_t> x : C)
		if (	has(S, abs(x.first.x)) &&
			has(S, abs(x.first.y)) &&
			has(S, abs(x.first.z)) &&
			has(S, abs(x.second)))
			f(x.first.x), f(x.first.y), f(x.first.z),
			x.first.rehash(), c.emplace(x.first, f(x.second));
	C = move(c);
	map<bools, unordered_map<array<int_t, 2>, int_t>, veccmp<bool>> cx;
	unordered_map<array<int_t, 2>, int_t> cc;
	for (const auto& x : CX) {
		for (pair<array<int_t, 2>, int_t> y : x.second)
			if (	has(S, abs(y.first[0])) &&
				has(S, abs(y.first[1])) &&
				has(S, abs(y.second)))
				f(y.first[0]), f(y.first[1]),
				cc.emplace(y.first, f(y.second));
		if (!cc.empty()) cx.emplace(x.first, move(cc));
	}
	CX = move(cx);
	map<pair<bools, uints>, unordered_map<array<int_t, 2>, int_t>,
		vec2cmp<bool, uint_t>> cxp;
	for (const auto& x : CXP) {
		for (pair<array<int_t, 2>, int_t> y : x.second)
			if (	has(S, abs(y.first[0])) &&
				has(S, abs(y.first[1])) &&
				has(S, abs(y.second)))
				f(y.first[0]), f(y.first[1]),
				cc.emplace(y.first, f(y.second));
		if (!cc.empty()) cxp.emplace(x.first, move(cc));
	}
	CXP = move(cxp);
	unordered_map<int_t, int_t> q;
	map<bools, unordered_map<int_t, int_t>, veccmp<bool>> mex;
	for (const auto& x : memos_ex) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mex.emplace(x.first, move(q));
	}
	memos_ex = move(mex);
	map<uints, unordered_map<int_t, int_t>, veccmp<uint_t>> mp;
	for (const auto& x : memos_perm) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mp.emplace(x.first, move(q));
	}
	memos_perm = move(mp);
	map<pair<uints, bools>, unordered_map<int_t, int_t>,
		vec2cmp<uint_t, bool>> mpe;
	for (const auto& x : memos_perm_ex) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mpe.emplace(x.first, move(q));
	}
	memos_perm_ex = move(mpe);
	bool b;
	map<bools, unordered_map<bdds, int_t>, veccmp<bool>> amx;
	for (const auto& x : AMX) {
		for (pair<bdds, int_t> y : x.second) {
			b = false;
			for (int_t& i : y.first)
				if ((b |= !has(S, abs(i)))) break;
				else f(i);
			if (!b && has(S, abs(y.second)))
				am.emplace(y.first, f(y.second));
		}
		if (!am.empty()) amx.emplace(x.first, move(am));
	}
	AMX = move(amx);
	map<pair<bools, uints>, unordered_map<bdds, int_t>,
		vec2cmp<bool, uint_t>> amxp;
	for (const auto& x : AMXP) {
		for (pair<bdds, int_t> y : x.second) {
			b = false;
			for (int_t& i : y.first)
				if ((b |= !has(S, abs(i)))) break;
				else f(i);
			if (!b && has(S, abs(y.second)))
				am.emplace(y.first, f(y.second));
		}
		if (!am.empty()) amxp.emplace(x.first, move(am));
	}
	AMXP = move(amxp);
	for (pair<bdds, int_t> x : AM) {
		b = false;
		for (int_t& i : x.first)
			if ((b |= !has(S, abs(i)))) break;
			else f(i);
		if (!b&&has(S,abs(x.second))) am.emplace(x.first, f(x.second));
	}
	AM=move(am), bdd_handle::update(p), Mp.resize(pvars), Mn.resize(nvars);
	p.clear(), S.clear();
	for (size_t n = 0; n < V.size(); ++n)
		if (V[n].v < 0)
			Mn[-V[n].v].emplace(bdd_key(hash_pair(V[n].h, V[n].l),
				V[n].h, V[n].l), n);
		else Mp[V[n].v].emplace(bdd_key(hash_pair(V[n].h, V[n].l),
				V[n].h, V[n].l), n);
	output::to(L"error") <<"AM: " << AM.size() << " C: "<< C.size() << endl;
}

void bdd_handle::update(const vector<int_t>& p) {
	std::unordered_map<int_t, std::weak_ptr<bdd_handle>> m;
	for (pair<int_t, std::weak_ptr<bdd_handle>> x : M) {
		DBG(assert(!x.second.expired());)
		spbdd_handle s = x.second.lock();
		f(s->b), m.emplace(f(x.first), x.second);
	}
	M = move(m);
}
#undef f

spbdd_handle bdd_handle::get(int_t b) {
	DBG(assert((size_t)abs(b) < V.size());)
	auto it = M.find(b);
	if (it != M.end()) return it->second.lock();
	spbdd_handle h(new bdd_handle(b));
	return	M.emplace(b, std::weak_ptr<bdd_handle>(h)), h;
}

void bdd::bdd_sz(int_t x, set<int_t>& s) {
	if (!s.emplace(x).second) return;
	bdd b = get(x);
	bdd_sz(b.h, s), bdd_sz(b.l, s);
}

spbdd_handle operator&&(cr_spbdd_handle x, cr_spbdd_handle y) {
	spbdd_handle r = bdd_handle::get(bdd::bdd_and(x->b, y->b));
	return r;
}

spbdd_handle operator%(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_and(x->b, -y->b));
}

spbdd_handle operator||(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_or(x->b, y->b));
}

spbdd_handle bdd_impl(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_or(-x->b, y->b));
}

bool bdd_subsumes(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd::bdd_subsumes(x->b, y->b);
}

spbdd_handle bdd_ite(cr_spbdd_handle x, cr_spbdd_handle y, cr_spbdd_handle z) {
	return bdd_handle::get(bdd::bdd_ite(x->b, y->b, z->b));
}

spbdd_handle bdd_ite_var(uint_t x, cr_spbdd_handle y, cr_spbdd_handle z) {
	return bdd_handle::get(bdd::bdd_ite_var(x, y->b, z->b));
}

spbdd_handle bdd_and_many(bdd_handles v) {
	if (V.size() >= gclimit) bdd::gc();
/*	if (v.size() > 16) {
		bdd_handles x, y;
		spbdd_handle r;
		for (size_t n = 0; n != v.size(); ++n)
			(n < v.size()>>1 ? x : y).push_back(v[n]);
		v.clear();
		r = bdd_and_many(move(x));
		return r && bdd_and_many(move(y));
	}*/
	bdds b;
	b.reserve(v.size());
	for (size_t n = 0; n != v.size(); ++n) b.push_back(v[n]->b);
	am_sort(b);
//	DBG( wcout<<"am begin"<<endl;
	auto r = bdd_handle::get(bdd::bdd_and_many(move(b)));
//	DBG( wcout<<"am end"<<endl;
	return r;
}

spbdd_handle bdd_and_many_ex(bdd_handles v, const bools& ex) {
	if (V.size() >= gclimit) bdd::gc();
	bool t = false;
	for (bool x : ex) t |= x;
	if (!t) return bdd_and_many(move(v));
	bdds b;
	b.reserve(v.size());
	for (size_t n = 0; n != v.size(); ++n) b.push_back(v[n]->b);
	am_sort(b);
	auto r = bdd_handle::get(bdd::bdd_and_many_ex(move(b), ex));
	return r;
}

spbdd_handle bdd_and_many_ex_perm(bdd_handles v, const bools& ex,
	const uints& p) {
	if (V.size() >= gclimit) bdd::gc();
//	DBG(assert(bdd_nvars(v) < ex.size());)
//	DBG(assert(bdd_nvars(v) < p.size());)
	bdds b;
	b.reserve(v.size());
	for (size_t n = 0; n != v.size(); ++n) b.push_back(v[n]->b);
	am_sort(b);
	auto r = bdd_handle::get(bdd::bdd_and_many_ex_perm(move(b), ex, p));
	return r;
}

int_t bdd_or_reduce(bdds b) {
	if (b.empty()) return F;
	if (b.size() == 1) return b[0];
	if (b.size() == 2) return bdd::bdd_or(b[0], b[1]);
	int_t t = F;
	if (b.size() & 1) t = b.back(), b.erase(b.begin()+b.size()-1);
	bdds x(b.size()>>1);
	for (size_t n = 0; n != x.size(); ++n)
		x[n] = bdd::bdd_or(b[n<<1], b[1+(n<<1)]);
	return bdd::bdd_or(t, bdd_or_reduce(move(x)));
}

spbdd_handle bdd_or_many(const bdd_handles& v) {
	bdds b(v.size());
	for (size_t n = 0; n != v.size(); ++n) b[n] = v[n]->b;
	return bdd_handle::get(bdd_or_reduce(move(b)));
/*	int_t r = F;
	for (auto x : v) r = bdd::bdd_or(r, x->b);
	return bdd_handle::get(r);
	bdds b(v.size());
	for (size_t n = 0; n != v.size(); ++n) b[n] = -v[n]->b;
	return bdd_handle::get(-bdd::bdd_and_many(move(b)));*/
}

void bdd::sat(uint_t v, uint_t nvars, int_t t, bools& p, vbools& r) {
	if (t == F) return;
	if (!leaf(t) && v < var(t))
		p[v - 1] = true, sat(v + 1, nvars, t, p, r),
		p[v - 1] = false, sat(v + 1, nvars, t, p, r);
	else if (v != nvars) {
		p[v - 1] = true, sat(v + 1, nvars, hi(t), p, r),
		p[v - 1] = false, sat(v + 1, nvars, lo(t), p, r);
	} else	r.push_back(p);
}

vbools bdd::allsat(int_t x, uint_t nvars) {
	bools p(nvars);
	vbools r;
	return sat(1, nvars + 1, x, p, r), r;
}

vbools allsat(cr_spbdd_handle x, uint_t nvars) {
	return bdd::allsat(x->b, nvars);
}

void allsat_cb::sat(int_t x) {
	if (x == F) return;
	const bdd bx = bdd::get(x);
	if (!bdd::leaf(x) && v < bdd::var(x)) {
		DBG(assert(bdd::var(x) <= nvars);)
		p[++v-2] = true, sat(x), p[v-2] = false, sat(x), --v;
	} else if (v != nvars + 1)
		p[++v-2] = true, sat(bx.h),
		p[v-2] = false, sat(bx.l), --v;
	else f(p, x);
}

int_t bdd::bdd_ex(int_t x, const bools& b, unordered_map<int_t, int_t>& memo,
	int_t last) {
	if (leaf(x) || (int_t)var(x) > last+1) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	DBG(assert(var(x)-1 < b.size());)
	if (b[var(x) - 1]) return bdd_ex(bdd_or(hi(x), lo(x)), b, memo, last);
	return memo.emplace(x, bdd::add(var(x), bdd_ex(hi(x), b, memo, last),
				bdd_ex(lo(x), b, memo, last))).first->second;
}

int_t bdd::bdd_ex(int_t x, const bools& b) {
	int_t last = 0;
	for (size_t n = 0; n != b.size(); ++n) if (b[n]) last = n;
	return bdd_ex(x, b, memos_ex[b], last);
}

spbdd_handle operator/(cr_spbdd_handle x, const bools& b) {
	return bdd_handle::get(bdd::bdd_ex(x->b, b));
}

int_t bdd::bdd_permute(const int_t& x, const uints& m,
		unordered_map<int_t, int_t>& memo) {
	if (leaf(x) || m.size() <= var(x)-1) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	return memo.emplace(x, bdd_ite_var(m[var(x)-1],
		bdd_permute(hi(x), m, memo),
		bdd_permute(lo(x), m, memo))).first->second;
}

spbdd_handle operator^(cr_spbdd_handle x, const uints& m) {
//	DBG(assert(bdd_nvars(x) < m.size());)
	return bdd_handle::get(bdd::bdd_permute(x->b, m, memos_perm[m]));
}

int_t bdd::bdd_permute_ex(int_t x, const bools& b, const uints& m, size_t last,
	unordered_map<int_t, int_t>& memo) {
	if (leaf(x) || var(x) > last+1) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	int_t t = x, y = x;
	DBG(assert(b.size() >= var(x));)
	for (int_t r; var(y)-1 < b.size() && b[var(y)-1]; y = r)
		if (leaf((r = bdd_or(hi(y), lo(y)))))
			return memo.emplace(t, r), r;
		DBG(else assert(b.size() >= var(r));)
	DBG(assert(!leaf(y) && m.size() >= var(y));)
	return	memo.emplace(t, bdd_ite_var(m[var(y)-1],
		bdd_permute_ex(hi(y), b, m, last, memo),
		bdd_permute_ex(lo(y), b, m, last, memo))).first->second;
}

int_t bdd::bdd_permute_ex(int_t x, const bools& b, const uints& m) {
	size_t last = 0;
	for (size_t n = 0; n != b.size(); ++n) if (b[n] || (m[n]!=n)) last = n;
	return bdd_permute_ex(x, b, m, last, memos_perm_ex[{m,b}]);
}

spbdd_handle bdd_permute_ex(cr_spbdd_handle x, const bools& b, const uints& m) {
//	DBG(assert(bdd_nvars(x) < b.size());)
//	DBG(assert(bdd_nvars(x) < m.size());)
	return bdd_handle::get(bdd::bdd_permute_ex(x->b, b, m));
}

spbdd_handle bdd_and_ex_perm(cr_spbdd_handle x, cr_spbdd_handle y,
	const bools& b, const uints& m) {
//	DBG(assert(bdd_nvars(x) < b.size());)
//	DBG(assert(bdd_nvars(x) < m.size());)
//	DBG(assert(bdd_nvars(y) < b.size());)
//	DBG(assert(bdd_nvars(y) < m.size());)
	return bdd_handle::get(bdd::bdd_and_ex_perm(x->b, y->b, b, m));
}

spbdd_handle bdd_and_ex(cr_spbdd_handle x, cr_spbdd_handle y,
	const bools& b) {
//	DBG(assert(bdd_nvars(x) < b.size());)
//	DBG(assert(bdd_nvars(y) < b.size());)
//	out(wcout, x)<<endl<<endl;
//	out(wcout, y)<<endl<<endl;
	return bdd_handle::get(bdd::bdd_and_ex(x->b, y->b, b));
}

spbdd_handle bdd_and_not_ex(cr_spbdd_handle x, cr_spbdd_handle y,
	const bools& b) {
//	DBG(assert(bdd_nvars(x) < b.size());)
//	DBG(assert(bdd_nvars(y) < b.size());)
	return bdd_handle::get(bdd::bdd_and_ex(x->b, -y->b, b));
}

spbdd_handle bdd_and_not_ex_perm(cr_spbdd_handle x, cr_spbdd_handle y,
	const bools& b, const uints& m) {
//	DBG(assert(bdd_nvars(x) < b.size());)
//	DBG(assert(bdd_nvars(y) < b.size());)
//	DBG(assert(bdd_nvars(x) < m.size());)
//	DBG(assert(bdd_nvars(y) < m.size());)
	return bdd_handle::get(bdd::bdd_and_ex_perm(x->b, -y->b, b, m));
}

spbdd_handle from_bit(uint_t b, bool v) {
	return bdd_handle::get(bdd::from_bit(b, v));
}

spbdd_handle from_eq(uint_t x, uint_t y) {
	return bdd_ite(from_bit(x,true), from_bit(y,true), from_bit(y,false));
}

void bdd::bdd_nvars(int_t x, set<int_t>& s) {
	if (!leaf(x)) s.insert(var(x)-1), bdd_nvars(hi(x),s),bdd_nvars(lo(x),s);
}

size_t bdd::bdd_nvars(int_t x) {
	if (leaf(x)) return 0;
	set<int_t> s;
	bdd_nvars(x, s);
	size_t r = *s.rbegin();
	return r;
}

size_t bdd_nvars(bdd_handles x) {
	size_t r = 0;
	for (auto y : x) r = max(r, bdd_nvars(y));
	return r;
}

size_t bdd_nvars(spbdd_handle x) { return bdd::bdd_nvars(x->b); }
bool leaf(cr_spbdd_handle h) { return bdd::leaf(h->b); }
bool trueleaf(cr_spbdd_handle h) { return bdd::trueleaf(h->b); }
wostream& out(wostream& os, cr_spbdd_handle x) { return bdd::out(os, x->b); }

size_t std::hash<ite_memo>::operator()(const ite_memo& m) const {
	return m.hash;
}

size_t std::hash<array<int_t, 2>>::operator()(const array<int_t, 2>& x) const {
	return hash_pair(x[0], x[1]);
}

size_t std::hash<bdd_key>::operator()(const bdd_key& k) const {return k.hash;}

size_t std::hash<bdds>::operator()(const bdds& b) const {
	size_t r = 0;
	for (int_t i : b) r ^= i;
	return r;
}

bdd::bdd(int_t v, int_t h, int_t l) : h(h), l(l), v(v) {
//	DBG(assert(V.size() < 2 || (v && h && l));)
}

wostream& bdd::out(wostream& os, int_t x) {
	if (leaf(x)) return os << (trueleaf(x) ? L'T' : L'F');
	const bdd b = get(x);
	return out(out(os << b.v << L" ? ", b.h) << L" : ", b.l);
}
