/*
 * Copyright (C) 2026 Ed Arvelaez
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "Event.t.h"

#include <catch2/catch_test_macros.hpp>

using namespace sel;

#define SEL_TEST_GROUP "sel.Event."

TEST_CASE(SEL_TEST_GROUP "basic execution") {
    Event<128, int> e;
    int sum = 0;
    e.subscribe([&](int v, auto /*self*/) { sum += v; });
    e.subscribe([&](int v, auto /*self*/) { sum += 2 * v; });
    e.invoke(2);

    REQUIRE(sum == 6);
}

TEST_CASE(SEL_TEST_GROUP "unsubscribe during execution") {
    Event<128, int> e;
    int calls = 0;

    auto handlerOne = e.subscribe([&](int, auto self) {
        calls++;
        self.unsubscribe();
    });

    auto handlerTwo = e.subscribe([&](int, auto /*self*/) {
        calls++;
    });

    e.invoke(1);
    e.invoke(1);

    REQUIRE(calls == 3);
    REQUIRE(handlerOne.isSubscribed() == false);
	REQUIRE(handlerTwo.isSubscribed() == true);
}

TEST_CASE(SEL_TEST_GROUP "max capacity enforcement") {
    Event<2, int> e;
    e.subscribe([](int, auto /*self*/){});
    e.subscribe([](int, auto /*self*/){});

    REQUIRE_THROWS(e.subscribe([](int, auto /*self*/){}));
}


TEST_CASE(SEL_TEST_GROUP "compaction allows reuse after cleanup") {
    Event<4, int> e;
    auto h1 = e.subscribe([](int, auto /*self*/){});
    auto h2 = e.subscribe([](int, auto /*self*/){});
    auto h3 = e.subscribe([](int, auto /*self*/){});
    h2.unsubscribe();

    e.invoke(1);
	REQUIRE(h1.isSubscribed() == true);
	REQUIRE(h2.isSubscribed() == false);
	REQUIRE(h3.isSubscribed() == true);
    REQUIRE_NOTHROW(e.subscribe([](int, auto /*self*/){}));
}

TEST_CASE(SEL_TEST_GROUP "self-subscription during invocation does not execute immediately") {
    Event<128, int> e;
    int calls = 0;
    Event<128, int>::Handler innerHandler;

    auto outerHandler = e.subscribe([&](int, auto self) {
        calls++;

        innerHandler = e.subscribe([&](int, auto /*inner_self*/) {
            calls++;
        });

        self.unsubscribe(); // prevent accumulation effects
    });

    e.invoke(1);
	REQUIRE(outerHandler.isSubscribed() == false);
	REQUIRE(innerHandler.isSubscribed() == true);
    REQUIRE(calls == 2);

    e.invoke(1);
    REQUIRE(calls == 3);
}

TEST_CASE(SEL_TEST_GROUP "Event throws on re-entrant invocation") {
    Event<128, int> e;
    bool inner_called = false;

    e.subscribe([&](int, auto /*self*/) {
        // This attempts re-entrant invocation
        inner_called = true;
        REQUIRE_THROWS_AS(e.invoke(42), std::runtime_error);
    });

    e.invoke(1);

    REQUIRE(inner_called);
}

#undef SEL_TEST_GROUP