/// @file	
///	@ingroup 	minapi
///	@copyright	Copyright (c) 2016, Cycling '74
///	@license	Usage of this file and its contents is governed by the MIT License

#pragma once

#include <sstream>

namespace c74 {
namespace min {
	
	
	class inlet;
	class outlet;
	class method;
	class attribute_base;
	
	template <typename T>
	class attribute;
	
	
	class maxobject_base {
	public:
		
		operator max::t_object*() {
			return &objstorage.maxobj;
		}

		operator max::t_pxobject*() {
			return &objstorage.mspobj;
		}

		operator void*() {
			return &objstorage.maxobj;
		}

		
	private:
		union storage {
			max::t_object	maxobj;
			max::t_pxobject	mspobj;
		};
		
		//enum class type {
		//	max = 0,
		//	msp,
		//};
		//
		//type objtype = type::max;
		storage objstorage;
		
	};
	
	
	/// An object_base is a generic way to pass around a min::object as a min::object, while sharing common code,
	/// is actually sepcific to the the user's defined class due to template specialization.
	class object_base {
	public:
		object_base()
		: m_state((max::t_dictionary*)k_sym__pound_d)
		{}
		
		virtual ~object_base() {
		}

		int current_inlet() {
			return proxy_getinlet((max::t_object*)m_maxobj);
		}
		
		operator max::t_object* () {
			return m_maxobj;
		}
		
		void assign_instance(max::t_object* instance) {
			m_maxobj = instance;
		}
		
		auto inlets() -> std::vector<min::inlet*>& {
			return m_inlets;
		}
		
		auto outlets() -> std::vector<min::outlet*>& {
			return m_outlets;
		}

		auto methods() -> std::unordered_map<std::string, method*>& {
			return m_methods;
		}
		
		auto attributes() -> std::unordered_map<std::string, attribute_base*>& {
			return m_attributes;
		}
		
		void preinitialize() {
			m_initializing = true;
		}

		void postinitialize() {
			m_initialized = true;
			m_initializing = false;
		}
		
		bool initialized() {
			return m_initialized;
		}
		
		dict state() {
			return m_state;
		}
		
		/// Try to call a named method.
		/// @param	name	The name of the method to attempt to call.
		/// @param	args	Any args you wish to pass to the method call.
		/// @return			If the method doesn't exist an empty set of atoms.
		///					Otherwise the results of the method.
		atoms try_call(const std::string& name, const atoms& args = {});

		/// Try to call a named method.
		/// @param	name	The name of the method to attempt to call.
		/// @param	arg		A single atom arg you wish to pass to the method call.
		/// @return			If the method doesn't exist an empty set of atoms.
		///					Otherwise the results of the method.
		atoms try_call(const std::string& name, const atom& arg) {
			atoms as = {arg};
			return try_call(name, as);
		}

	protected:
		max::t_object*										m_maxobj;
		bool												m_initializing = false;
		bool												m_initialized = false;
		std::vector<min::inlet*>							m_inlets;
		std::vector<min::outlet*>							m_outlets;
		std::unordered_map<std::string, method*>			m_methods;
		std::unordered_map<std::string, attribute_base*>	m_attributes;
		dict												m_state;
	};
	
	
	

	
	
	class sample_operator_base;
	class perform_operator_base;
	class matrix_operator_base;
	class gl_operator_base;
	
	
	// Wrap the C++ class together with the appropriate Max object header
	// Max object header is selected automatically using the type of the base class.
	
	template<class T, class=void>
	struct minwrap;
	
	
	template<class T>
	struct minwrap < T, typename std::enable_if<
		!std::is_base_of< min::perform_operator_base, T>::value
		&& !std::is_base_of< min::sample_operator_base, T>::value
		&& !std::is_base_of< min::gl_operator_base, T>::value
	>::type > {
		maxobject_base	base;
		T				obj;
		
		void setup() {
			auto self = &base;
			auto inlets = obj.inlets();

			for (auto i=inlets.size()-1; i>0; --i)
				inlets[i]->instance = max::proxy_new(self, i, nullptr);
		}
		
		void cleanup() {}
	};
	
	
#pragma mark -
#pragma mark inlets / outlets

	class port {
	public:
		port(object_base* an_owner, std::string a_description, std::string a_type)
		: owner(an_owner)
		, description(a_description)
		, type(a_type)
		{}
		
		bool has_signal_connection() {
			return signal_connection;
		}
		
		object_base*	owner;
		std::string		description;
		std::string		type;
		bool			signal_connection = false;
	};
	
	
	class inlet : public port {
	public:
		inlet(object_base* an_owner, std::string a_description, std::string a_type = "")
		: port(an_owner, a_description, a_type) {
			owner->inlets().push_back(this);
		}

		void* instance = nullptr;
	};
	
	
	class outlet  : public port {
	public:
		outlet(object_base* an_owner, std::string a_description, std::string a_type = "")
		: port(an_owner, a_description, a_type) {
			owner->outlets().push_back(this);
		}
		
		void send(double value) {
			c74::max::outlet_float(instance, value);
		}

		void send(symbol s1) {
			c74::max::outlet_anything(instance, s1, 0, nullptr);
		}

		void send(std::string s1) {
			c74::max::outlet_anything(instance, c74::max::gensym(s1.c_str()), 0, nullptr);
		}

		void send(const char* s1) {
			c74::max::outlet_anything(instance, c74::max::gensym(s1), 0, nullptr);
		}

		void send(symbol s1, symbol s2) {
			atom a(s2);
			c74::max::outlet_anything(instance, s1, 1, &a);
		}

		void send(symbol s1, double f2) {
			atom a(f2);
			c74::max::outlet_anything(instance, s1, 1, &a);
		}
		
		void* instance = nullptr;
	};
	
#ifdef __APPLE__
# pragma mark -
# pragma mark methods
#endif

	using function = std::function<atoms(const atoms&)>;
	#define MIN_FUNCTION [this](const c74::min::atoms& args) -> c74::min::atoms

	
	class method {
	public:
		method(object_base* an_owner, std::string a_name, function a_function)
		: owner(an_owner)
		, function(a_function)
		{
			if (a_name == "integer")
				a_name = "int";
			else if (a_name == "number")
				a_name = "float";
			else if (a_name == "dsp64" || a_name == "dblclick" || a_name == "edclose" || a_name == "okclose" || a_name == "patchlineupdate")
				type = max::A_CANT;
			owner->methods()[a_name] = this;
		}
		
		atoms operator ()(atoms args = {}) {
			return function(args);
		}
		
		atoms operator ()(atom arg) {
			return function({arg});
		}
		
		//private:
		object_base*	owner;
		long			type = max::A_GIMME;
		function		function;
	};

	
	using setter = function;
	using getter = std::function<atoms()>;;

	
	class attribute_base {
	public:
		attribute_base(object_base& an_owner, std::string a_name)//, function a_function)
		: m_owner(an_owner)
		, m_name(a_name)
        , m_title(a_name)
		{}
		
		virtual attribute_base& operator = (atoms args) = 0;
		virtual operator atoms() const = 0;

		symbol datatype() {
			return m_datatype;
		}
		
		const char* label_string() {
			return m_title;
		}
		
		virtual const char* range_string() = 0;
		
	protected:
		object_base&	m_owner;
		symbol			m_name;
        symbol			m_title;
		symbol			m_datatype;
		function		m_setter;
		function		m_getter;
	};
	
	
	using title = std::string;
	using range = atoms;

	
	template <typename T>
	class attribute : public attribute_base {
		
		
		
		
	private:
		
		
		/// constructor utility: handle an argument defining an attribute's digest / label
		template <typename U>
		constexpr typename std::enable_if<std::is_same<U, title>::value>::type
		assign_from_argument(const U& arg) noexcept {
			m_title = arg;
		}
		
		/// constructor utility: handle an argument defining a parameter's range
		template <typename U>
		constexpr typename std::enable_if<std::is_same<U, range>::value>::type
		assign_from_argument(const U& arg) noexcept {
			for (const auto& a : arg)
				m_range.push_back(a);
		}
		
		/// constructor utility: handle an argument defining a parameter's setter function
		template <typename U>
		constexpr typename std::enable_if<std::is_same<U, setter>::value>::type
		assign_from_argument(const U& arg) noexcept {
			m_setter = arg;
		}

		/// constructor utility: handle an argument defining a parameter's setter function
		template <typename U>
		constexpr typename std::enable_if<std::is_same<U, getter>::value>::type
		assign_from_argument(const U& arg) noexcept {
			m_getter = arg;
		}

		/// constructor utility: empty argument handling (required for recursive variadic templates)
		constexpr void handle_arguments() noexcept {
			;
		}
		
		/// constructor utility: handle N arguments of any type by recursively working through them
		///	and matching them to the type-matched routine above.
		template <typename FIRST_ARG, typename ...REMAINING_ARGS>
		constexpr void handle_arguments(FIRST_ARG const& first, REMAINING_ARGS const& ...args) noexcept {
			assign_from_argument(first);
			if (sizeof...(args))
				handle_arguments(args...);		// recurse
		}
		
		
	public:
		/// Constructor
		/// @param an_owner			The instance pointer for the owning C++ class, typically you will pass 'this'
		/// @param a_name			A string specifying the name of the attribute when dynamically addressed or inspected.
		/// @param a_default_value	The default value of the attribute, which will be set when the instance is created.
		/// @param ...args			N arguments specifying optional properties of an attribute such as setter, label, style, etc.
		template <typename...ARGS>
		attribute(object_base* an_owner, std::string a_name, T a_default_value, ARGS...args)
		: attribute_base(*an_owner, a_name) {
			m_owner.attributes()[a_name] = this;

			if (std::is_same<T, bool>::value)				m_datatype = k_sym_long;
			else if (std::is_same<T, int>::value)			m_datatype = k_sym_long;
			else if (std::is_same<T, symbol>::value)		m_datatype = k_sym_symbol;
			else if (std::is_same<T, float>::value)			m_datatype = k_sym_float32;
			else /* (std::is_same<T, double>::value) */		m_datatype = k_sym_float64;

			handle_arguments(args...);
			
			*this = a_default_value;
		}
		
		
		attribute& operator = (const T arg) {
			atoms as = { atom(arg) };
			*this = as;
			if (m_owner.initialized())
				object_attr_touch(m_owner, (max::t_symbol*)m_name);
			return *this;
		}
		
		
		attribute& operator = (atoms args) {
			if (m_setter)
				m_value = m_setter(args)[0];
			else
				m_value = args[0];
			return *this;
		}
		
		
		operator atoms() const {
			atom a = m_value;
			atoms as = { a };
			return as;
		}
		
		
		operator T() const {
			return m_value;
		}
		
		
		const char* range_string() {
			std::stringstream ss;
			for (const auto& val : m_range)
				ss << val << " ";
			return ss.str().c_str();
		};

		
	private:
		T				m_value;
		std::vector<T>	m_range;
	};
	
	
	
	
	atoms object_base::try_call(const std::string& name, const atoms& args) {
		auto meth = m_methods.find(name);
		if (meth != m_methods.end())
			return (*meth->second)(args);
		return {};
	}

	
	
	
	static max::t_symbol* ps_getname = max::gensym("getname");
	

	
	template<class T>
	max::t_max_err min_attr_getter(minwrap<T>* self, max::t_object* maxattr, long* ac, max::t_atom** av) {
		max::t_symbol* attr_name = (max::t_symbol*)max::object_method(maxattr, ps_getname);
		auto& attr = self->obj.attributes()[attr_name->s_name];
		atoms rvals = *attr;
		
		*ac = rvals.size();
		if (!(*av)) // otherwise use memory passed in
			*av = (max::t_atom*)max::sysmem_newptr(sizeof(max::t_atom) * *ac);
		for (auto i=0; i<*ac; ++i)
			(*av)[i] = rvals[i];
		
		return 0;
	}
	
	
	template<class T>
	max::t_max_err min_attr_setter(minwrap<T>* self, max::t_object* maxattr, atom_reference args) {
		max::t_symbol* attr_name = (max::t_symbol*)max::object_method(maxattr, ps_getname);
		auto& attr = self->obj.attributes()[attr_name->s_name];
		*attr = atoms(args.begin(), args.end());
		return 0;
	}

	
	
	
	// maxname may come in as an entire path because of use of the __FILE__ macro
	std::string deduce_maxclassname(const char* maxname) {
		std::string smaxname;
		
		const char* start = strrchr(maxname, '/');
		if (start)
			start += 1;
		else {
			start = strrchr(maxname, '\\');
			if (start)
				start += 1;
			else
				start = maxname;
		}
		
		const char* end = strstr(start, "_tilde.cpp");
		if (end) {
			smaxname.assign(start, end-start);
			smaxname += '~';
		}
		else {
			const char* end = strrchr(start, '.');
			if (!end)
				end = start + strlen(start);
			if (!strcmp(end, ".cpp"))
				smaxname.assign(start, end-start);
			else
				smaxname = start;
		}
		return smaxname;
	}
	
}} // namespace c74::min
