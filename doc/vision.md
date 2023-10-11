# Vision and Motivation

## Motivation

We live in a world where software is increasingly complex, and increasingly fragile as a result. It's very
easy to end up in a situation where software that was working just fine a few months ago can no longer
compile and run due to broken dependencies. One of the main goals of UVM is to combat the phenomenon
known as "code rot" or "bit rot". UVM will provides simple, minimalistic and robust APIs. Most importantly,
after a certain experimentation period, existing APIs will be essentially frozen and guaranteed to remain
stable. That will ensure that software built to run on UVM will have a much better chance of working,
hopefully years or even decades into the future.

The idea of a virtual machine is obviously not new. We're all familiar with the JVM. You can also think
of the web platform as a VM. The key differences are that UVM will be much smaller, and that its APIs will
be much simpler. UVM isn't going to provide complex, high-level APIs like a GUI toolkit, because different
implementations of such high-level APIs would be almost guaranteed to behave differently, in the same way that
modern web browsers such as Firefox, Safari and Chrome can't produce an identical rendering of the
same webpage. When the API surface is too big, there is too much room for interpretation and for small but
material behavioral differences to creep in.

There seems to be a growing interest in retrocomputing, and that interest likely stems, in large part,
because the complexity of modern computer systems and their software environment is extremely high, and
there is constant unnecessary churn, which becomes exhausting. At some point, programmers just want to point,
and there is a natural desire to declutter and have fun. UVM aims to provide a platform that is simple,
stable over time, and easy to target and develop for. I hope that this platform will help bring back the joy
of programming for many.

If you'd like to read more about the philosophy behind UVM's design, here are some relevant blog posts:
- [Building a Minimalistic Virtual Machine](https://pointersgonewild.com/2023/02/24/building-a-minimalistic-virtual-machine/)
- [Code that Doesn't Rot](https://pointersgonewild.com/2022/02/11/code-that-doesnt-rot/)
- [Typed vs Untyped Virtual Machines](https://pointersgonewild.com/2022/06/08/typed-vs-untyped-virtual-machines/)
- [The Need for Stable Foundations in Software Development](https://pointersgonewild.com/2020/09/22/the-need-for-stable-foundations-in-software-development/)
- [Minimalism in Programming](https://pointersgonewild.com/2018/02/18/minimalism-in-programming/)

## Vision

I envision UVM as a platform that would remain very stable over time. It's designed to be small, and
to expose only APIs that have a small surface area, so that the VM will be easy to maintain, easy
to port to new platforms, and have as few undefined behaviors as possible.

The APIs provided by UVM are in some ways very spartan. You can't link to external
functions using an FFI. You get a frame buffer, keyboard and mouse input, network sockets and a
device to output audio samples and not much more than that. Fundamentally though, what does
software need to interface with users and with the outside world? It needs inputs and outputs.
UVM tries to provide input and output APIs that are simple, will behave predictably and can be
guaranteed to remain stable over time. The idea is that if you can program software using only
those APIs, it should be relatively easy to keep your software running even 100 years from now.

As UVM evolves, new APIs will need be added, but, after it reaches 1.0 (which we are not at yet),
we will keep the existing APIs essentially frozen. The hope is that this will free developers from
much of the stress of modern software development. With UVM, you can be sure that the foundations your
software is built on aren't going to constantly shift under your feet for seemingly no reason.
