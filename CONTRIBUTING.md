# Contributing Guide

> ⚠️ **Note:** This documentation is a work-in-progress and, quite ironically, needs contributions!

This documentation outlines how to contribute to the FlightGear project. If you're looking for information on how to get involved with FlightGear, you're in the right place.

[TOC]

## How to use Git effectively

- Make _atomic_ git commits whenever possible. Individual commits should be small, self-contained, and focused on one thing. This makes commits easier to work with, review, and cherry-pick.
- Use Merge Requests to combine multiple commits into logical sets of changes that accomplish something meaningful.

## How to use GitLab effectively
- Use issues to organize your work, coordinate, and broadcast your intention to others.
- When creating an issue or merge request, you should _always_ use a description template if one is available and applicable.
    - For example, if you're filing a bug report, you should use the bug report description template when you create an issue.
- If you are making large changes, break those changes up into multiple merge requests that each solve a logical part of the problem. Massive merge requests become very difficult to review.

### How to use labels for GitLab Issues and Merge Requests

For organizational and project management purposes, [labels](https://docs.gitlab.com/ee/user/project/labels.html) are used on issues and merge requests. This makes it easy to sort, filter, and organize issues or merge requests. Additionally, [scoped labels](https://docs.gitlab.com/ee/user/project/labels.html#scoped-labels) provide even more control and organization. Most all of the labels in the FlightGear project are scoped labels.

You should use (scoped) labels as thoroughly as possible in your issues and merge requests. This will improve organization and make your work easier to find and manage for other developers. That developer might be the one who fixes the bug you reported or approves your merge request, so it pays to ensure labels are used and used accurately.

Scoped labels allow pertinent information to be obtained at a glance. For example, without even opening an issue or reading its title, simply looking at the `type` scoped label will indicate whether the issue is a bug report, suggestion, etc., based on its value (e.g., `type::bug` indicates the issue is a bug report, not a discussion, suggestion, or support request). Another example is the `system` scoped label, which indicates what system or feature in FlightGear an issue or merge request pertains to. A scoped label of `system::canvas` instantly communicates that the item is related to FlightGear's 2D rendering framework, known as Canvas.

This is particularly useful when sorting through dozens of issues or merge requests to find what you're looking for. Furthermore, developers can opt to [receive notifications](https://docs.gitlab.com/ee/user/project/labels.html#receive-notifications-when-a-label-is-used) when a particular label is applied. This would be especially helpful for someone maintaining a particular feature, as they can stay informed about issues and merge requests related to their area of responsibility.

### C++ Coding Style

FlightGear has a large codebase containing contributions from many people over an extended period. This has led to a lack of a standard coding style, but rest assured it is being addressed and progress is being made.

1. Brand-new code _should always_ be formatted with `clang-format`.
2. When modifying _existing code_, use discretion when changing the pre-existing coding style to match the 'new' style. Will the resulting diff be difficult to understand? Will it disrupt git history? If you plan to heavily refactor a source file such that it will already be disruptive, that may be a good time to include formatting changes.

### Nasal Coding Style

Nasal, the built-in scripting language for FlightGear, does not have a defined coding style, either by FlightGear developers or the language creator. As a bespoke language that is not used outside of FlightGear, no linters, formatters, or other code quality tools exist for it.

## How to contribute code

> 💡 **Note:** You'll need to familiarize yourself with setting up a development environment and building FlightGear in order to contribute code. See the [building guide](BUILDING.md) if you do not already have a working setup.

1. **Ensure you have read the sections above:**
   If you haven't read the following sections of documentation above, you will likely encounter needless frustration:

   1. [How to use Git effectively](#c-coding-guidelines)
   2. [How to use GitLab effectively](#how-to-use-gitlab-effectively)
   3. [C++ Coding Style](#c++-coding-style)
   4. [Nasal Coding Style](#nasal-coding-style)

1. **Fork the Repository:**
   Fork the relevant FlightGear repository into your GitLab account. You will need to know which repository contains the code you are trying to change. Familiarizing yourself with the codebase and the purpose of each repository is strongly recommended.

2. **Build the Project:**
   If you haven't already done so, now is the time to setup your development environment so that you can build FlightGear. See the [building guide](BUILDING.md) for more information.

2. **Create a New Branch:**
   Create a branch on your fork to isolate your changes. Use a clear and descriptive name for your branch (e.g., `fix-menubar-typo` or `add-new-joystick-configs`).

3. **Make Changes:**
   Make the changes on your branch in small, focused commits. Each commit should be self-contained and represent one logical change. Push your changes to your branch as needed.

4. **Review:**
   Before you submit your changes to the FlightGear team for review, double-check
   your work. Ensure that your code builds and that tests pass.

5. **Submit a Merge Request (MR):**
   Open a Merge Request (MR) to propose your changes for review. Be sure to use the **Merge Request Template** to provide necessary context for reviewers.

6. **Code Review:**
   Once you have created a Merge Request, other developers will review your work, give feedback, and sometimes request changes be made. This is a crucial
   part of the development process, as it helps reduce bugs and improves code quality.
