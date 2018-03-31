cc_library(
    name = 'AF',
    srcs = [
        'actor.cpp',
        'address.cpp',
        'allocator_manager.cpp',
        'detail/handlers/default_handler_collection.cpp',
        'detail/handlers/fallback_handler_collection.cpp',
        'detail/handlers/handler_collection.cpp',
        'detail/strings/string_pool.cpp',
        'detail/threading/clock.cpp',
        'framework.cpp',
        'receiver.cpp',
    ],
    deps = [
    ],
    defs = [
        '_GLIBCXX_USE_NANOSLEEP',
        '_GLIBCXX_USE_SCHED_YIELD'
    ],
    extra_cppflags = [
        '-fPIC',
        '-std=c++11',
    ]
)
