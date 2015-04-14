#pragma once

#include <boost/coroutine/all.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "base/logging.hpp"

typedef boost::function<void(void)> fn;
typedef boost::coroutines::coroutine< void(fn*) > coro_t;
typedef coro_t::caller_type caller_t;

namespace rrr{
class CoroPool{
	void reg_ca(caller_t* ca){
		_ca = ca;
	}
	void reg_ca(){
		coro_t* c = _c;
		caller_t* ca = _ca;

		if (callee_map.find(c) == callee_map.end()){
			callee_map[c] = ca;
		}
		verify(callee_map[c] == ca);

		if (caller_map.find(ca) == caller_map.end()){
			caller_map[ca] = c;
		}
		verify(caller_map[ca] == c);
	}

	void release(caller_t* ca){
		coro_t* c = caller_map[ca];
		_pool.push_back(c);
	}

	void main_loop(caller_t& ca){
		reg_ca(&ca);
		ca.get();

		ca();
		
		while(true){
			auto f = ca.get();
			if (f == NULL){
				break;
			}else{
				(*f)();
				release(&ca);
				ca();
			}
		}
	}

	std::map<coro_t *, caller_t *> callee_map;
	std::map<caller_t *, coro_t *> caller_map;
	coro_t* _c;
	caller_t* _ca;

	std::vector<coro_t*> _pool;

	coro_t* alloc_coro(){
		coro_t* c = _pool.back();
		_pool.pop_back();
		return c;
	}

public:
	void init(int size=1000){
		for (int i=0; i<size; i++){
			coro_t* c = new coro_t(boost::bind(&CoroPool::main_loop, this, _1), 0);
			_c = c;
			reg_ca();
			_pool.push_back(c);
		}
	}
	void reg_function(fn* f){
		coro_t* c = alloc_coro();
		(*c)(f);
	}
	void yeild(){
		(*_ca)();
	}
	void yeildto(caller_t* ca){
		coro_t* c = caller_map[ca];
		_ca = ca;
		(*c)(0);
	}

	void release(){
		for (auto &c: _pool){
			(*c)(0);
		}
	}
};

}