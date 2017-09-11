#include "Throutine.h"
#include <tuple>
#include <iostream>
#include <thread>
#include <chrono>


void test1()
{
    auto sleep = []()
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);
    };
    try
    {
        struct G
        {
            ~G()
            {
                __debugbreak();
            }
        } g;
        Throutine routine([]()
        {
            Throutine::accept<void>();
            int a = 0;
            int b = 1;
            for (int i = 0;; ++i)
            {
                if (i == 6)
                    throw 1;
                std::tie(a, b) = std::make_tuple(b, a + b);
                Throutine::yield(a);
            }
        });
        sleep();
        for (int i = 0; i < 10; ++i)
        {
            std::cout << routine.call<int>() << std::endl;
            sleep();
        }
    }
    catch (...)
    {
        __debugbreak();
    }
    sleep();
}


void test2()
{
    __debugbreak();
    for (int i = 0; i < 1000000; ++i)
    {
        Throutine routine([]()
        {
            Throutine::accept<void>();
            //Throutine::yield((int)(1));//this is very slow
            Throutine::reply(int(1));
        });
        routine.call<int>();
    }
    __debugbreak();
}


int main()
{
	test1();
	test2();
	return 0;
}