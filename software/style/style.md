Coding Style Guidelines
=======================

## Table of Contents

* **[Whitespace and Indentation](#whitespace-and-indentation)**
* **[Names](#names)**
* **[Order of Class Members](#order-of-class-members)**
* **[Miscellaneous Language Features](#miscellaneous-language-features)**
* **[Pointers](#pointers)**
* **[Documentation](#documentation)**
	* **[Commenting Conventions](#commenting-conventions)**
* **[Spelling](#spelling)**
* **[Line Width](#line-width)**
* **[Headers](#headers)**

Whitespace and Indentation
-------------------------
Good spacing and indentation will make your code easier to read to others. Below are general examples of how to do so.


####Template for correct indentation

```
class MyClass {
	// An example of a table in a comment.
	//
	// Byte Offset | Length | Value
	//      0      |    4   | Packet ID
	//      4      |    4   | Packet Length
	//      8      |    n   | Packet Payload
	//  ^       ^    ^    ^
	//  \-------+----+----+-- These are all spaces...
	// ... but to the left of the comment slashes are tabs.

	public:
		MyClass() {
			// statements
		}

		int function() {
			switch (x) {
				case 0:
				case 1:
					// statements
					break;

				case 2:
					// statements
					do_stuff();
					break;
			}

			return (x + 1) * 2 + 7;
		}

	private:
		int x;
	};
```

####Example of correct indentation
Taken from *ai/hl/stp/tactic/idle.cpp*
```c++
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/action/stop.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

//FILE DESCRIPTION: Renders robot idle.

namespace {
	class Idle final : public Tactic {
		public:
			explicit Idle(World world) : Tactic(world) { }

// REFERENCE: Tactic superclass for override methods descriptions.

		private:
			
			Player select(const std::set<Player> &players) const override;

			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"idle";
			}
	};


	Player Idle::select(const std::set<Player> &players) const {
		return *players.begin(); // returns first element/robot from player vector
	}

// executes caller on selected player

	void Idle::execute(caller_t& caller) {
		while (true) {
			caller();
		}
	}
}
// creates idle world for selected player by calling superclass
Tactic::Ptr AI::HL::STP::Tactic::idle(World world) {
	return Tactic::Ptr(new Idle(world));
}
```


Names
-----

Names of types (such as classes, structs, unions, enums, "typename"-type template parameters, and typedefs) as well as namespaces shall be in camel case with a leading capital letter, such as MyClass.

Names of compile-time constants (such as #defines (though these should be avoided if possible), static const class members, const globals, and enum elements) shall be in all capitals with underscores separating words, such as MY_CONSTANT.

Names of global variables (including file-scope globals), non-const static members, const non-static members, parameters, local variables, and functions (member or not, static or not) shall be in all lowercase with underscores separating words, such as my_function.

Names of files shall be in all lowercase with underscores separating words (and shall match the name of their main contained class, if they contain a class) and shall end with ".h" or ".cpp", such as my_class.h and my_class.cpp.



Order of Class Members
----------------------

Within a class, first have a "public:" tag, then all the public members, then the same for protected and private.

Within an access level, you **must** list members in this order:

* Typedefs
* Constants (static const members)
* Variables
* Static functions and operators
* Constructors
* Destructors
* Member functions and operators

There should never be a private static function in a class body. As such a function is not associated with an instance of the class and cannot be called from anywhere other than the file implementing the class, such a function should be made file-scope, that is, placed within the anonymous namespace in the source file.



Miscellaneous Language Features
-------------------------------

Avoid the use of C-style casts ("(int) x") and function-style casts ("int(x)") at all costs. Casting should be avoided where possible and, if strictly necessary, should be done via the static_cast, dynamic_cast, const_cast, and reinterpret_cast keywords (the latter two should also be avoided strongly unless there is a clear need for their use).

Avoid std::string for storing strings that are related to user input or output. Use Glib::ustring for these to allow proper handling of Unicode code points.

Ask around before implementing something that feels like it should have been implemented by someone else already. It might already have been.

If you're doing something that relies on variables having specific sizes, use the C99-style types provided in <cstdint>: int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, and uint64_t.

If you're converting between integers and pointers, don't. If you're still converting between integers and pointers, use uintptr_t or intptr_t on the integer side. Note that it's still illegal to store pointers to functions or pointers to class members in these (just as it's illegal to store pointers to functions or pointers to class members in variables of type void*).

Primitives (char/short/int/long/intN_t/float/double/pointers) should generally be passed to functions by value. Non-primitives should generally be passed by const reference. Parameters used to return data should be passed by nonconst reference.

An attempt should be made to ensure that classes follow basic const-correctness rules. This is more important when objects have value semantics than when they have reference semantics.

When writing constructors, non-static member variables shall as much as possible be initialized through the use of initializer lists, not through assignment statements in the constructor body. It is permitted to omit the initializer list entry for an object member whose default constructor is desired, but remember that primitive members are NOT zero-filled unless explicitly mentioned in the initializer list. If extensive code is needed to compute the initial value of a member, consider whether that code should be moved to a file-scope global function which can itself be invoked in the initializer list.

All constructors shall be marked with the “explicit” keyword. In the case of a one-argument constructor, this prevents it from being used as an implicit type conversion; in the case of other constructors, it acts as a safeguard in case arguments are later added or removed. This rule shall be ignored for constructors that actually SHOULD be used as implicit conversions; by default, however, add the keyword.

When a function returns an object, you need to keep the object around for the duration of the scope that invoked the function but no longer, and you do not need to perform any non-const operations on the object, use a const reference to the return value:

	void caller() {
		const Foo &foo = callee();
		foo.do_something();
	}

This causes the lifetime of the return value, "foo", to be extended to the end of "callee"'s scope, but may potentially avoid unnecessary copies of the object from temporaries. This does not work with a non-const reference.



Pointers
--------

Raw pointers shall be avoided unless absolutely unavoidable.

If your object needs to be created in a scope, live its life, and die at the end of the scope (possibly the lifetime of an enclosing object), make it extend NonCopyable (to prevent it from being copied unnecessarily) and instantiate it as a local variable or class member.

If your object needs to be put into a container or provided as a return value but is lightweight and has value semantics, consider making it copyable (by not extending NonCopyable) and using it by value.

If your object can be tied to a single scope but needs to be created and destroyed at intermediate points in time during the lifetime of that scope (e.g. class member that needs to be destroyed and recreated during the life of the object) or needs to be substitutable with subclasses of itself, consider extending NonCopyable and having users of your class store ScopedPtrs to it. Remember that ScopedPtrs are themselves NonCopyable and thus cannot be stored in containers.

If your object needs reference semantics and full ability to be passed around and stored in many locations simultaneously, make your class extend ByRef, make your constructors private, and provide static create() functions that return Glib::RefPtr<YourClass>.

The correct way to write such static create functions is:

	Glib::RefPtr<YourClass> YourClass::create(int x, int y, int z) {
		Glib::RefPtr<YourClass> p(new YourClass(x, y, z));
		return p;
	}

Do not try to combine the constructor invocation with the return statement. It is easy to accidentally violate exception safety and create a memory leak by trying to combine multiple statements here. It is better to make the constructor invocation a single complete statement.



Documentation
-------------

Our first draft of documentation is generated using **Doxygen**. This will prove to be of use to you in understanding how certain cpp files will work. To install it, open a terminal window and type:
```
$ sudo apt-get install doxygen
```
Then in trunk/software type
```
$ make doc
```
and documentation will be genereated in an html folder.

However, in order for this to work properly you **must properly comment your code**. All public and protected types, functions, variables, and constants shall be **documented in Doxygen style**.

####Commenting Conventions

* **File Description**: when starting a new file, write a sentence or two (at most) outlining what it does and contains. This should be short enough such that when a teammate glances at the document, they should know what they are looking at. For example, for the shoot.cpp file found in trunk/software/ai/hl/stp/tactic/shoot.cpp the description should look like:

```cpp
// FILE DESCRIPTION: Robot shoots at target with specific coordinates.
``` 
* **Functions**: at the beginning of each new function, comment what the **purpose** of your function is, what **parameters** it takes, and what it **returns**. Note that this should only be for **new** functions that you yourself create. If you are using already existing functions outlined in the superclass, there is no need to comment them. If you are introducing special behaviour within methods, comment them using single or multi-line comments (see below).
```
/**
 * Performs the foo operation on this Bazquux.
 *
 * \param[in] x the X coordinate of the operation.
 *
 * \param[in] y the Y coordinate of the operation.
 *
 * \return the fooed Bazquux.
 */
```

* **Single-line comment**
```
// This is a comment about the following line.
```
* **Multi-line comment** (you can also split the code out into a separate function in this case and Doxygenizing it)

```
/*
 * This is the first line of a long comment.
 * This is a second line.
 */
``` 

* **The TODO style**: This can be useful in reminding yourself what needs to be completed. This may go outside a function or inside. TODO comments should be for **yourself** only. When you commit your code, there should be no more TODOs left.
```cpp
// TODO: do something
```
* **Warning style**: This is especially handy if you're running into compiler errors. It is more apparent to other programmers to know where debugging needs to be done.
```cpp
#warning: something doesn't work
```



If you're running into compiler errors or your code fails testing, try to avoid writing "TODO:" comments. Instead, use the #warning preprocessor directive so that the need for fixing is more apparent to others.

*Note*: The single-line Doxygen comment style introduced by /// shall **not** be used.

Spelling
--------

Use Canadian spelling, which is often British but occasionally American. Examples, "colour", "neighbour", and "honour", but "aluminum" (not "aluminium").

Exceptions:
Use "defense" in lieu of "defence" (and similarly "offense"). Rationale: similarity to "defensive", etc.



Line Width
----------

For regular source code, no maximum line width is mandated. Hard line breaks within statements shall not be used; instead, the line shall be as wide as necessary to accomodate the statement. If the line looks too large, it should probably be broken into multiple statements.

For comments (including Doxygen comments), each sentence should start on a new line (in Doxygen comments, paragraphs are set apart by two line breaks). A single sentence should generally occupy a single line, unless it becomes very large (e.g. larger than around 150 columns), in which case it should be wrapped at the most natural breaking point (e.g. after a semicolon or a comma or before or after a parenthetical).



Headers
-------

Header files need guards to prevent their contents from being included more than once in a way that would cause compile errors. The traditional way to do this is to put "include guards" around the contents of every header file. In our project, an include guard's name should be the full path of the header file, with forward slashes replaced by underscores and fully capitalized. 

For example, a header file called "ai/robot_controller/robot_controller.h" would look like this:
```c++
#ifndef AI_ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
// Contents here
#endif
```


When a file includes headers, the order of #include statements should obey the
following rules, from **most to least** important:

* If this is a .cpp file that includes its own corresponding .h file, that #include should be first (rationale: this ensures that the .h file itself #includes everything it depends upon).

* All #includes of project header files (which use quotes, as in #include "ai/robot_controller/robot_controller.h") should come before all #includes of system header files (which use angle brackets, as in #include <glibmm.h>).

* All files should be named as though by an alphabetical preorder tree traversal: files in a parent directory should come before files in subdirectories, files in the same directory should be alphabetically ordered, and subdirectories below a parent directory should be alphabetically ordered.

Furthermore, all #includes of project header files should use full paths
relative to the software directory, not relative to the #including file.

As an example, the following sequence of #includes is correct for the file
"ai/robot_controller/robot_controller.cpp", with comments explaining:
```
#include "ai/robot_controller/robot_controller.h" // rule 1
#include "ai/flags.h" // parent before children
#include "ai/util.h" // alphabetical within a directory
#include "ai/robot_controller/util.h" // child after parents
#include "ai/robot_controller/world.h" // alphabetical within a directory
#include <algorithm> // system headers after project headers
#include <cassert> // alphabetical within a directory
#include <glibmm.h> // alphabetical within a directory
#include <vector> // alphabetical within a directory
#include <sigc++/sigc++.h> // child after parents
```


Automatic Formatting
====================

Some of the rules described in this style guide (though not all of them) can be applied to source code automatically. To assist with this, the file "uncrustify.cfg" is available, which is a configuration file for the "uncrustify" source-code-formatting tool. Once the tool is installed, run "make uncrustify" to automatically invoke the tool against all files in the repository (other than those in proto/ (which are auto-generated)).

The current release version of uncrustify as of this writing, 0.56, contains a few bugs. One such bug is fixed by GIT commit 9a25a81a035dd7c1fe3d25213be14b482d48465d, which adds a previously-missing newline between the access level specification at the very start of a class body and a constructor (or possibly other function) immediately following it. The other is believed not to be fixed as of this writing, and deals with spaces around ampersands in reference-type parameters to function declarations in class bodies. Therefore, please be careful to run "svn stat" and "svn diff" before committing the output of a "make uncrustify", to ensure that such rules have not been rebroken by the tool.
