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
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <climits>
#include <stdexcept>
#include <cassert>
#include "rule.h"
#ifdef DEBUG
#include "driver.h"
#else
#include "lp.h"
#endif
using namespace std;

#define err_goalsym "goal symbol not appearing in program.\n"
#define err_goalarity "goal arity larger than the program's.\n"

void tml_init() { bdd_init(); }
DBG(wostream& printbdd(wostream& os, size_t t);)

void lp::add_fact(size_t f, int_t rel, ints arity) {
	size_t *t = db[{rel, arity}];
	if (!t) *(db[{rel, arity}] = new size_t) = f;
	else *t = bdd_or(*t, f);
}

bool lp::add_not_fact(size_t f, int_t rel, ints arity) {
	size_t *t = db[{rel, arity}], p = *t;
	return (*t = bdd_and_not(*t, f)), (p == F || *t != F);
}

bool lp::add_fact(const term& x) {
//	if (x.neg) return add_not_fact(fact(x, bits, dsz), x.rel, x.arity);
	return add_fact(fact(x, bits, dsz), x.rel, x.arity), true;
}

bool lp::add_facts(const matrix& x) {
	for (auto y : x) if (!add_fact(y)) return false; // FIXME
	return true;
}

lp::lp(matpairs r, matrix g, int_t delrel, size_t dsz, const strs_t& strs,
	lp *prev) : prev(prev), bits(msb(dsz)), dsz(dsz), delrel(delrel)
	, strs(strs) {
	//wcout<<r<<endl;
	for (const auto& m : r) {
		for (const term& t : m.first)
			*(db[{t.rel, t.arity}] = new size_t) = F;
		for (const term& t : m.second)
			*(db[{t.rel, t.arity}] = new size_t) = F;
	}
	for (const auto& m : r)
 		if (m.second.empty()) {
			if (!add_facts(m.first))
				// FIXME
//				(wcout << L"contradictory fact: "<<m[0]<<endl),
				exit(0);
		} else {
			vector<size_t*> dbs;
			for (size_t n = 0; n < m.second.size(); ++n)
//				if (m.second[n].b == term::NONE)
				dbs.push_back(db[{m.second[n].rel,
					m.second[n].arity}]);
			rules.emplace_back(
				new rule(m.first, m.second, dbs, bits, dsz));
		}
//	DBG(printdb(wcout<<L"pos:"<<endl, this);)
//	DBG(printndb(wcout<<L"neg:"<<endl, this)<<endl;)
	for (const term& t : g) {
		if (t.arity.size() > 2 && !t.arity[0] && t.arity[1] == -1) {
			trees.emplace(diff_t::key_type{t.rel, t.arity}, 
					fact(t, bits, dsz));
			DBG(drv->printdiff(wcout<<"trees:"<<endl, trees);)
		} else gbdd = bdd_or(gbdd, fact(t, bits, dsz));
	}
}

void lp::fwd(diff_t &add, diff_t &del) {
	//DBG(printdb(wcout, this));
	for (rule* r : rules) {
		const sizes x = r->fwd(bits);
		if (x.empty()) continue;
		for (size_t n = 0; n != x.size(); ++n) {
			size_t &t = (r->neg[n] ? del : add)
				[{r->hrel[n], r->harity[n]}];
			t = bdd_or(x[n], t);
		}
	}
	//DBG(printdiff(wcout<<"add:"<<endl,add););
	//DBG(printdiff(wcout<<"del:"<<endl,del););
	//DBG(printdb(wcout<<"after step: "<<endl, this)<<endl;)
}

struct diffcmp {
	bool operator()(const lp::diff_t& x, const lp::diff_t& y) const {
		if (x.size() != y.size()) return x.size() < y.size();
		auto xt = x.begin(), yt = y.begin();
		while (xt != x.end())
			if (xt->first != yt->first) return xt->first<yt->first;
			else if (xt->second == yt->second) ++xt, ++yt;
			else return xt->second < yt->second;
		return false;
	}
};

lp::diff_t copy(const lp::db_t& x) {
	lp::diff_t r;
	for (auto y : x) r[y.first] = *y.second;
	return r;
}

void copy(const lp::diff_t& src, lp::db_t& dst) {
	for (auto x : dst) delete x.second;
	dst.clear();
	for (auto x : src) dst.emplace(x.first, new size_t(x.second));
}

bool bdd_and_not(const lp::diff_t& x, const lp::diff_t& y, lp::diff_t& r) {
	for (auto t : x) {
		auto it = y.find(t.first);
		if (it == y.end()) continue;
		if (t.second && F == (r[t.first] 
				= bdd_and_not(t.second, y.at(t.first))))
			return false;
	}
	return true;
}

lp::db_t& bdd_and_not(lp::db_t& x, const lp::diff_t& y) {
	for (auto t : x) {
		auto it = y.find(t.first);
		if (it == y.end()) continue;
		*t.second = bdd_and_not(*t.second, y.at(t.first));
	}
	return x;
}

void bdd_or(lp::db_t& x, const lp::diff_t& y) {
	for (auto t : x) {
		auto it = y.find(t.first);
		if (it == y.end()) continue;
		*t.second = bdd_or(*t.second, y.at(t.first));
	}
}

bool operator==(const lp::db_t& x, const lp::diff_t& y) {
	if (x.size() != y.size()) return false;
	auto xt = x.begin();
	auto yt = y.begin();
	while (xt != x.end())
		if (xt->first != yt->first || *xt->second != yt->second)
			return false;
		else ++xt, ++yt;
	return true;
}

bool lp::pfp() {
	if (prev) {
		if (!prev->pfp()) return false;
		for (auto x : prev->db)
			if (db.find(x.first) != db.end())
				*db[x.first] = bdd_or(*db[x.first], *x.second);
			else *(db[x.first] = new size_t) = *x.second;
//		align(prev->db, prev->bits, bits);
	}
	diff_t d, add, del, t;
	set<size_t> pf;
	size_t step = 0;
//	wcout << V.size() << endl;
	for (set<diff_t, diffcmp> s;;) {
		wcout << "step: " << step++ << endl;
		s.emplace(d = copy(db)), fwd(add, del);
		if (!bdd_and_not(add, del, t))
			return false; // detect contradiction
		for (auto x : add)
			add_fact(x.second, x.first.first, x.first.second);
		for (auto x : del)
			add_not_fact(x.second, x.first.first, x.first.second);
		if (db == d) break;
		if (s.find(copy(db)) != s.end()) return false;
	}
	DBG(drv->printdiff(wcout<<"trees:"<<endl, trees);)
	if (trees.empty()) goto ret;
	for (auto x : trees)
		for (auto p : prefix(db, x.first.second, x.first.first)) {
			auto it = db.find({x.first.first, p});
			if (it != db.end()) {
				size_t y = bdd_expand(x.second,
					arlen(x.first.second),
					arlen(it->first.second), bits);
				y = bdd_and(*it->second, y);
				get_tree(x.first.first, y, it->first.second,
					db, bits, trees_out);
			}
		}
	copy(trees_out, db);
ret:	if (delrel != -1) {
		set<pair<int_t, ints>> d;
		for (auto x : db) if (x.first.first==delrel) d.insert(x.first);
		for (auto x : d) db.erase(x);
	}
	DBG(static int nprog = 0;)
	DBG(printdb(wcout<<"after prog: "<<nprog++<<endl, this)<<endl;)
//	if (gbdd != F) db = bdd_and(gbdd, db);
	return true;
}

lp::~lp() {
//	for (rule* r : rules) delete r;
//	if (prev) delete prev;
//	if (proof1) delete proof1, delete proof2;
}
