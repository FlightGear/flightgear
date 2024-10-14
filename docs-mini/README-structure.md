# Strucutral Overview

## Foundation Components


There is a group os basic objects used widely throughout the codebase, which
are useful building blocks in all major simulation elements. 

### Properties 

Properties form a hierarchy of named, typed values. The hierarchy can be extended
and modified at runtime by aircraft, simulation components and external protocols.

### Listeners

Listeners are a callback interface `SGPropertyChangeListener` which can implemented
to receive notification when a property is modified, or its children added/removed.
Most commonly listeners are used to look for a user input or simulation event
changing some value (for example, a switch being turned from on to off), but it's
possible to monitor an entire sub-tree of properties.

It's important than listeners do not block or perform excessive amounts of work, since
the code modifying the property may not expect this. If non-trivial work needs to be
performed in response to a property changing, the listener should set a dirty flag
and a subsystem (etc) should check this in its `update()` method.

### Tied properties

Tied properties allow a property to be defined by a C++ memory location (such as a class
member variable) or by callback functions implementation the get/set operations. This
is very convenient for the implementing code. However, by default it has the
problematic side-effect that listeners do not correctly, since they have no way to be
notified that the underlying value (or result of invoking the getter) has changed.

It is therefore critical, when using tied properties, to _manually_ invoke `fireValueChanged`
on dependent properties, when updating the internal state producing them. Care should be 
taken to only fire the change, when the result of the getters would actually change; it is
_not_ acceptable to simply call `fireValueChanged` each iteration, since any connected
listener will be run at the simulation rate.

### Conditions

_Conditions_ are  predicates which can be evaluated on demand. They can contain
logical operations such as 'and' and 'or', and comparisons: for example checking that
a property is equal to a constant value, or that a property is less than another
property.

Conditions are widely used to drive other state from properties; for example
enabling or disabling user-interface in a dialog, or activating an autopilot
element.

### Bindings

_Bindings_ are a command invocation with associated arguments. In the simplest case
this might be invoking the `property-set` command with an argument of 42. At its
most complex, the command might invoke a Nasal script, supplied as part of the
binding, and substituting a runtime value into the Nasal logic.

Bindings are typically used in response to user actions, such as a menu item, key
press or interacting with a cockpit control, to update some part of the simulator
state.

## Structural Components

Most code within FlightGear is organized into subsystems. Subsystems are isolated from each other;
a well designed subsystem can be added, remove or restarting independently. Many subsystems do not
full conform to these goals, especially regarding runtime adding and removal, but an increasing
number do support it.

=== Subsystems ===

=== Commands ===


## Finding data files

## Subsystem Lifecycle
