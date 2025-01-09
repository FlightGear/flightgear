# Contributing Guide

> ⚠️ **Note:** This documentation is a work-in-progress and, quite ironically, needs contributions!

This documentation outlines how to contribute to the FlightGear project. If you're looking for information on how to get involved with FlightGear, you're in the right place.


## How To Contribute Code 
1. Fork the repository you wish to make changes to.
2. Create a new branch on your fork. This is where for your changes will be made.
3. Push commits to the branch on your fork.
4. When you're done, create a **Merge Request**.
    - Follow the "Merge Request Template"

### Git Best Practices
- Make _atomic_ git commits whenever possible. Individual commits should be small, self-contained and focused on one thing. This makes commits easier to work with and review.
- Use Merge Requests to combine multiple commits into logical sets of changes that accomplish something.


## Coding Guidelines

- Follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), co-authored by Bjarne Stroustrup, the creator of C++.

### C++ Coding Style

FlightGear has a large codebase containing contributions from many people over a large time period. This has led to a lack of a standard coding style, something that is being addressed, rest assured.

Brand new code _should always_ be formatted with `clang-format`.

When making modifications to _existing code_, use discretion in changing the pre-existing coding style to the 'new' coding style. Will the resulting diff be difficult to understand, will it disrupt git history? If you are going to heavily refactor a source file, such that it will already be disruptive anyways, that may be a good time to sneak in formatting changes.

### Nasal Coding Style

Nasal, the built-in scripting language for FlightGear, has no set coding style defined by either the FlightGear developers or the language creator. As a bespoke language that is not used outside of FlightGear, no linters, formatters, or other code quality tooling exist for it.

## Label System
For organizational and project management purposes, [labels](https://docs.gitlab.com/ee/user/project/labels.html) are used on issues and merge requests. This makes it easy to sort, filter, and organize issues or merge requests. Furthermore, [scoped labels](https://docs.gitlab.com/ee/user/project/labels.html#scoped-labels) enable even more control and organization in this regard. 

You should try and use labels as thoroughly as possible in your issues and merge requests - it will improve organization and make your work easier to find and manage by another developer. That developer might be the one who fixes the bug you reported or approves your merge request, so it pays to ensure labels are used, and used accurately. 

Scoped labels allow pertinent information to be obtained at a glance. For example, without even opening an issue or reading its title, simply looking at the `type` scoped label will indicate if the issue is a bug report, suggestion, etc based on its value (for example `type::bug` indicates the issue is a bug report, not a discussion, suggestion, or support request). Another example of a useful scoped label is `system`, which indicates what system or feature in FlightGear an issue or merge request pertains to. A scoped label of `system::canvas` instantly communicates that the item is related to FlightGear's 2D rendering framework, known as Canvas.

This is very useful when sorting through dozens of issues or merge requests to find what you're looking for. Further more, developers can opt to [receive notifications](https://docs.gitlab.com/ee/user/project/labels.html#receive-notifications-when-a-label-is-used) when a particular label has been used. This would be particularly useful for someone who is the maintainer of a particular feature, and wants to be made aware of issues and merge requests relating to the area of FlightGear they maintain.

