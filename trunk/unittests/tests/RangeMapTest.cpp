#include <muleunit/test.h>
#include <algorithm>
#include "RangeMap.h"


using namespace muleunit;

typedef CRangeMap<int*> TestRangeMap;


/**
 * Returns the contents of a TestRangeMap iterator as a string-presentation.
 */
wxString StringFrom(const TestRangeMap::const_iterator& it)
{
	return wxString::Format(wxT("(%llu, %llu, %li)"), it.keyStart(), it.keyEnd(), *it);
}

/**
 * Returns the contents of a TestRangeMap iterator as a string-presentation.
 */
wxString StringFrom(TestRangeMap::iterator it)
{
	return wxString::Format(wxT("(%llu, %llu, %li)"), it.keyStart(), it.keyEnd(), *it);
}


/**
 * Returns the contents of a CRangeSet iterator as a string-presentation.
 */
wxString StringFrom(const CRangeMap<void>::const_iterator& it)
{
	return wxString::Format(wxT("(%llu, %llu)"), it.keyStart(), it.keyEnd());
}

/**
 * Returns the contents of a CRangeSet iterator as a string-presentation.
 */
wxString StringFrom(CRangeMap<void>::iterator it)
{
	return wxString::Format(wxT("(%llu, %llu)"), it.keyStart(), it.keyEnd());
}

namespace muleunit {
	wxString StringFrom(int* value)
	{
		return wxString::Format(wxT("%li"), value);
	}
};


/**
 * Returns the contents of a TestRangeMap as a string-representation.
 *
 * Using this function allows for easy comparison against the expected 
 * result of a particular test.
 */
template <typename VALUE>
wxString StringFrom(const CRangeMap<VALUE>& map) 
{
	wxString stream;

	typename CRangeMap<VALUE>::const_iterator it = map.begin();
	for (; it != map.end(); ++it) {
		if (it != map.begin()) {
			stream << wxT(", ");
		}
		
		stream += StringFrom(it);
	}

	return wxT("[") + stream + wxT("]");
}



DECLARE(RangeMap);
	TestRangeMap m_map;
	TestRangeMap m_mmaps[3];

	// Identifers for the multirange maps
	enum Maps {
		CONT  = 0, // Continues ranges, IE no gap
		SSAME = 1, // Seperated but equal
		SDIFF = 2  // Seperated and not equal
	};

	// Sets up a few maps with predefined ranges.
	void setUp() {
		m_map.insert(100, 150, reinterpret_cast<int*>(0));
		ASSERT_EQUALS(wxT("[(100, 150, 0)]"), StringFrom(m_map));
		
		m_mmaps[CONT].insert(100, 150, reinterpret_cast<int*>(0));
		m_mmaps[CONT].insert(151, 200, reinterpret_cast<int*>(1));
		ASSERT_EQUALS(wxT("[(100, 150, 0), (151, 200, 1)]"), StringFrom(m_mmaps[CONT]));
		
		m_mmaps[SDIFF].insert(100, 150, reinterpret_cast<int*>(0));
		m_mmaps[SDIFF].insert(160, 200, reinterpret_cast<int*>(1));
		ASSERT_EQUALS(wxT("[(100, 150, 0), (160, 200, 1)]"), StringFrom(m_mmaps[SDIFF]));
		
		m_mmaps[SSAME].insert(100, 150, reinterpret_cast<int*>(1));
		m_mmaps[SSAME].insert(160, 200, reinterpret_cast<int*>(1));
		ASSERT_EQUALS(wxT("[(100, 150, 1), (160, 200, 1)]"), StringFrom(m_mmaps[SSAME]));
	}

	void tearDown() {
		m_map.clear();
		m_mmaps[0].clear();
		m_mmaps[1].clear();
		m_mmaps[2].clear();
	}


	/**
	 * Tests insertion into a map with a single range, checking against an
	 * expected result.
	 */
	void singleInsert(uint32 start, uint32 end, int value, const wxString& result)
	{
		TestRangeMap::iterator it = m_map.insert(start, end, reinterpret_cast<int*>(value));
		
		// Test that the correct iterator was returned
		ASSERT_TRUE(it.keyStart() <= start);
		ASSERT_TRUE(it.keyEnd() >= end);
		ASSERT_EQUALS(*it, reinterpret_cast<int*>(value));
		
		// Check the resulting map
		ASSERT_EQUALS(result, StringFrom(m_map));
		ASSERT_EQUALS((uint32)std::count(result.begin(), result.end(), '('), m_map.size());
		
		// Reset the rangemap
		tearDown();
		setUp();
	}	

	void doErase(uint32 start, uint32 end, const wxString& result)
	{
		m_map.erase_range(start, end);
		
		// Check the resulting map
		ASSERT_EQUALS(result, StringFrom(m_map));
		ASSERT_EQUALS((uint32)std::count(result.begin(), result.end(), '('), m_map.size());
		
		// Reset the rangemap
		tearDown();
		setUp();	
	}

	/** 
	 * Tests insertion into a map with multiple ranges, checking against an 
	 * expected result.
	 */
	void multiInsert(int type, uint32 start, uint32 end, int value, const wxString& result)
	{
		TestRangeMap::iterator it = m_mmaps[type].insert(start, end, reinterpret_cast<int*>(value));

		// Test that the correct iterator was returned
		ASSERT_TRUE(it.keyStart() <= start);
		ASSERT_TRUE(it.keyEnd() >= end);
		ASSERT_EQUALS(*it, reinterpret_cast<int*>(value));
		
		// Check the resulting map
		ASSERT_EQUALS(result, StringFrom(m_mmaps[type]));
		ASSERT_EQUALS((uint32)std::count(result.begin(), result.end(), '('), m_mmaps[type].size());
		
		tearDown();
		setUp();
	}
END_DECLARE;



TEST(RangeMap, DefaultConstructor)
{
	TestRangeMap map;

	ASSERT_EQUALS(0u, map.size());
	ASSERT_TRUE(map.empty());
}


TEST(RangeMap, CopyConstructor)
{
	// Check basic copying
	TestRangeMap mapA(m_map);
	TestRangeMap mapB(m_mmaps[CONT]);
	TestRangeMap mapC(m_mmaps[SSAME]);
	TestRangeMap mapD(m_mmaps[SDIFF]);
	
	ASSERT_EQUALS(m_map, mapA);
	ASSERT_EQUALS(m_mmaps[CONT], mapB);
	ASSERT_EQUALS(m_mmaps[SSAME], mapC);
	ASSERT_EQUALS(m_mmaps[SDIFF], mapD);	
	
	
	// The copies must not affect the originals
	mapA.insert(125, 175, reinterpret_cast<int*>(2));
	mapB.insert(125, 175, reinterpret_cast<int*>(2));
	mapC.insert(125, 175, reinterpret_cast<int*>(2));
	mapD.insert(125, 175, reinterpret_cast<int*>(2));

	ASSERT_FALSE(m_map == mapA);
	ASSERT_FALSE(m_mmaps[CONT] == mapB);
	ASSERT_FALSE(m_mmaps[SSAME] == mapC);
	ASSERT_FALSE(m_mmaps[SDIFF] == mapD);
}


TEST(RangeMap, AssignmentOperator)
{
	// Check basic assignment
	TestRangeMap mapA, mapB;
	mapA = mapB = m_map;
	
	ASSERT_EQUALS(m_map, mapA);
	ASSERT_EQUALS(m_map, mapB);
	

	// The copies must not affect the originals
	mapA.insert(125, 175, reinterpret_cast<int*>(2));
	mapB.insert(125, 175, reinterpret_cast<int*>(2));

	ASSERT_TRUE(mapA == mapB);
	ASSERT_FALSE(m_map == mapA);
	ASSERT_FALSE(m_map == mapB);
}


TEST(RangeMap, Equality)
{
	TestRangeMap mapA(m_mmaps[SSAME]), mapB(m_mmaps[SDIFF]);
	
	ASSERT_FALSE(mapA == mapB);
	
	mapA = mapB;

	ASSERT_EQUALS(mapA, mapB);

	mapA.insert(10, 20, reinterpret_cast<int*>(3));

	ASSERT_FALSE(mapA == mapB);
}


TEST(RangeMap, Iterators)
{
	// Adding a third range
	m_mmaps[CONT].insert(125, 175, reinterpret_cast<int*>(2));
	TestRangeMap map(m_mmaps[CONT]);
	
	TestRangeMap::iterator it = map.begin();
	TestRangeMap::iterator it_orig = map.begin();
	TestRangeMap::iterator it_other = map.end();	
	
	ASSERT_EQUALS(wxT("(100, 124, 0)"), StringFrom(it));
	
	it_other = ++it;
	
	ASSERT_EQUALS(wxT("(125, 175, 2)"), StringFrom(it));
	ASSERT_EQUALS(it_other, it++);
	ASSERT_EQUALS(wxT("(176, 200, 1)"), StringFrom(it));
	
	it_other = --it;
	
	ASSERT_EQUALS(wxT("(125, 175, 2)"), StringFrom(it));
	ASSERT_EQUALS(it_other, it--);
	ASSERT_EQUALS(wxT("(100, 124, 0)"), StringFrom(it));
}


TEST(RangeMap, Erase)
{
	// Adding a third range
	m_mmaps[CONT].insert(125, 175, reinterpret_cast<int*>(2));
	
	// The expected results from forwards deletion
	TestRangeMap map(m_mmaps[CONT]);
	wxString expectedFW[4];
	expectedFW[0] = wxT("[(100, 124, 0), (125, 175, 2), (176, 200, 1)]");
	expectedFW[1] = wxT("[(125, 175, 2), (176, 200, 1)]");
	expectedFW[2] = wxT("[(176, 200, 1)]");
	expectedFW[3] = wxT("[]");

	for (int i = 0; i < 4; ++i) {
		ASSERT_EQUALS(expectedFW[i], StringFrom(map));

		if (map.begin() != map.end()) {
			map.erase(map.begin());
		}
	}

	// Test the expected result from backwards deletion
	map = m_mmaps[CONT];
	wxString expectedBW[4];
	expectedBW[0] = wxT("[(100, 124, 0), (125, 175, 2), (176, 200, 1)]");
	expectedBW[1] = wxT("[(100, 124, 0), (125, 175, 2)]");
	expectedBW[2] = wxT("[(100, 124, 0)]");
	expectedBW[3] = wxT("[]");

	for (int i = 0; i < 4; ++i) {
		ASSERT_EQUALS(expectedBW[i], StringFrom(map));
		
		if (map.begin() != map.end()) {
			map.erase(--map.end());
		}
	}
}


TEST(RangeMap, Clear)
{
	m_map.clear();
	ASSERT_TRUE(m_map.empty());
	ASSERT_EQUALS(0u, m_map.size());
	ASSERT_EQUALS(wxT("[]"), StringFrom(m_map));
	
	for (int i = 0; i < 3; ++i) {
		m_mmaps[i].clear();
		ASSERT_TRUE(m_mmaps[i].empty());
		ASSERT_EQUALS(0u, m_mmaps[i].size());
		ASSERT_EQUALS(wxT("[]"), StringFrom(m_mmaps[i]));
	}
}


TEST(RangeMap, FindRange)
{
	// Adding a third range
	m_mmaps[CONT].insert(125, 175, reinterpret_cast<int*>(2));
	TestRangeMap map(m_mmaps[CONT]);
	
	ASSERT_EQUALS(map.end(), map.find_range(0));
	ASSERT_EQUALS(map.end(), map.find_range(98));
	ASSERT_EQUALS(map.end(), map.find_range(99));

	ASSERT_EQUALS(wxT("(100, 124, 0)"), StringFrom(map.find_range(100)));
	ASSERT_EQUALS(wxT("(100, 124, 0)"), StringFrom(map.find_range(112)));
	ASSERT_EQUALS(wxT("(100, 124, 0)"), StringFrom(map.find_range(124)));

	ASSERT_EQUALS(wxT("(125, 175, 2)"), StringFrom(map.find_range(125)));
	ASSERT_EQUALS(wxT("(125, 175, 2)"), StringFrom(map.find_range(150)));
	ASSERT_EQUALS(wxT("(125, 175, 2)"), StringFrom(map.find_range(175)));

	ASSERT_EQUALS(wxT("(176, 200, 1)"), StringFrom(map.find_range(176)));
	ASSERT_EQUALS(wxT("(176, 200, 1)"), StringFrom(map.find_range(186)));
	ASSERT_EQUALS(wxT("(176, 200, 1)"), StringFrom(map.find_range(200)));
	
	ASSERT_EQUALS(map.end(), map.find_range(201));
	ASSERT_EQUALS(map.end(), map.find_range(202));
	ASSERT_EQUALS(map.end(), map.find_range(250));
}


TEST(RangeMap, InvalidErase)
{
	ASSERT_RAISES(CInvalidParamsEx, m_map.erase(m_map.end()));
}


TEST(RangeMap, InvalidInsert)
{
	ASSERT_RAISES(CInvalidParamsEx, m_map.insert(10, 9, reinterpret_cast<int*>(8)));	
}


/////////////////////////////////////////////////
// The following tests exercize the merging algorithm.
// The comment before each comment descrices the RangeMaps used to test,
// and which types of ranges are tested. For example, the description
// "a / b <-> c / d" is to be read as: 
//   Insert range starting at a or b, ending at c or d.


// Single insert before start <-> before start.
TEST(RangeMap, SingleInsert_BeforeStart_BeforeStart)
{
	// Test with same and different type
	singleInsert(0, 90, 0, wxT("[(0, 90, 0), (100, 150, 0)]"));
	singleInsert(0, 90, 1, wxT("[(0, 90, 1), (100, 150, 0)]"));
}


// Single insert before start <-> touching start.
TEST(RangeMap, SingleInsert_BeforeStart_TouchingStart)
{
	singleInsert(0, 99, 0, wxT("[(0, 150, 0)]")); // Same type
	singleInsert(0, 99, 1, wxT("[(0, 99, 1), (100, 150, 0)]")); // Different type
}


// Single insert before start <-> at start / inside.
TEST(RangeMap, SingleInsert_BeforeStart_AtStart_InSide)
{
	for (int offset = 0; offset < 3; ++offset) { // at start, inside, inside
		singleInsert(0, 100 + offset, 0, wxT("[(0, 150, 0)]")); // Same type

		wxString expected = wxString::Format(wxT("[(0, %u, 1), (%u, 150, 0)]"), 100 + offset, 101 + offset);
		singleInsert(0, 100 + offset, 1, expected); // Different type
	}
}


// Single insert before start / touching start <-> at end / touching end / after end.
TEST(RangeMap, SingleInsert_BeforeStart_TouchingStart_AtEnd_TouchingEnd_AfterEnd)
{
	// Test with same and different type
	for (int type = 0; type < 2; ++type) {
		// (at end, touching end, after end)
		for (int end_offset = 0; end_offset < 3; ++end_offset) {
			// (before start, touching start)
			for (int start_offset = 0; start_offset < 2; ++start_offset) {
				wxString expected = wxString::Format(wxT("[(%u, %u, %li)]"), 98 + start_offset, 150 + end_offset, type);
				singleInsert(98 + start_offset, 150 + end_offset, type, expected);
			}
		}
	}
}


// Single insert touching start <-> touching start.
TEST(RangeMap, SingleInsert_TouchingStart_TouchingStart) 
{
	singleInsert(99, 99, 0, wxT("[(99, 150, 0)]")); // Same
	singleInsert(99, 99, 1, wxT("[(99, 99, 1), (100, 150, 0)]")); // Different
}


// Single insert touching start / at start <-> at start / inside.
TEST(RangeMap, SingleInsert_TouchingStart_AtStart_AtStart_Inside)
{
	for (int offset_a = 0; offset_a < 2; ++offset_a) { // (touching start, at start)
		for (int offset_b = 0; offset_b < 2; ++offset_b) { // (at start, inside)
			int start = 99 + offset_a;
			int end   = 100 + offset_b;
			
			// Same
			singleInsert(start, end, 0, wxString::Format(wxT("[(%u, 150, 0)]"), 99 + offset_a)); 

			// Different
			wxString exp = wxString::Format(wxT("[(%u, %u, 1), (%u, 150, 0)]"), start, end, 101 + offset_b);
			singleInsert(start, end, 1, exp);
		}
	}
}


// Single insert at start <-> at end / touching end / after end.
TEST(RangeMap, SingleInsert_AtStart_AtEnd_TouchingEnd_AfterEnd) 
{
	// (at end, touching end, after end)
	for (int offset = 0; offset < 3; ++offset) {
		// Same
		singleInsert(100, 150 + offset, 0, wxString::Format(wxT("[(100, %u, 0)]"), 150 + offset));
		// Different
		singleInsert(100, 150 + offset, 1, wxString::Format(wxT("[(100, %u, 1)]"), 150 + offset)); 
	}
}


// Single insert inside / at end <-> at end / touching end / after end.
TEST(RangeMap, SingleInsert_Inside_AtEnd_AtEnd_TouchingEnd_AfterEnd)
{
	// (inside, at end)
	for (int offset_a = 0; offset_a < 2; ++offset_a) {
		// (at end, touching end, after end)
		for (int offset_b = 0; offset_b < 3; ++offset_b) {
			int start = 149 + offset_a;
			int end   = 150 + offset_b;
			
			// Same
			singleInsert(start, end, 0, wxString::Format(wxT("[(100, %u, 0)]"), end));
			
			// Different
			singleInsert(start, end, 1, wxString::Format(wxT("[(100, %u, 0), (%u, %u, 1)]"), 148 + offset_a, start, end));
		}
	}
}


// Single insert touching end <-> touching end / after end.
TEST(RangeMap, SingleInsert_TouchingEnd_TouchingEnd_AfterEnd)
{
	// (touching end, after end)
	for (int offset = 0; offset < 2; ++offset) {
		int end = 151 + offset;
		
		// Same
		singleInsert(151, end, 0, wxString::Format(wxT("[(100, %u, 0)]"), end));
		
		// Different
		singleInsert(151, end, 1, wxString::Format(wxT("[(100, 150, 0), (151, %u, 1)]"), end));
	}
}


// Single insert after end <-> after end.
TEST(RangeMap, SingleInsert_AfterEnd_AfterEnd)
{
	singleInsert(152, 170, 0, wxT("[(100, 150, 0), (152, 170, 0)]")); // Same
	singleInsert(152, 170, 1, wxT("[(100, 150, 0), (152, 170, 1)]")); // Different
}


// Multi insert before start <-> before start.
TEST(RangeMap, MultiInsert_BeforeStart_BeforeStart)
{
	for (int type = 0; type < 2; ++type) { // Test with same and different type
		multiInsert(CONT, 0, 90, type, wxString(wxT("[(0, 90, ")) << type << wxT("), (100, 150, 0), (151, 200, 1)]"));
		multiInsert(SDIFF, 0, 90, type, wxString(wxT("[(0, 90, ")) << type << wxT("), (100, 150, 0), (160, 200, 1)]"));
		multiInsert(SSAME, 0, 90, type, wxString(wxT("[(0, 90, ")) << type << wxT("), (100, 150, 1), (160, 200, 1)]"));
	}
}


// Multi insert before start <-> touching start.
TEST(RangeMap, MultiInsert_BeforeStart_TouchingStart)
{
	multiInsert(CONT, 0, 99, 0, wxT("[(0, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 0, 99, 0, wxT("[(0, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 0, 99, 0, wxT("[(0, 99, 0), (100, 150, 1), (160, 200, 1)]"));
	
	multiInsert(CONT, 0, 99, 1, wxT("[(0, 99, 1), (100, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 0, 99, 1, wxT("[(0, 99, 1), (100, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 0, 99, 1, wxT("[(0, 150, 1), (160, 200, 1)]"));
}


// Multi insert before start <-> at start / inside.
TEST(RangeMap, MultiInsert_BeforeStart_AtStart_InSide)
{
	for (int offset = 0; offset < 3; ++offset) { // (at start, inside, inside)
		int end = 100 + offset;
		
		multiInsert(CONT, 0, end, 0, wxT("[(0, 150, 0), (151, 200, 1)]"));
		multiInsert(SDIFF, 0, end, 0, wxT("[(0, 150, 0), (160, 200, 1)]"));
		multiInsert(SSAME, 0, end, 0, wxString::Format(wxT("[(0, %u, 0), (%u, 150, 1), (160, 200, 1)]"), end, 101 + offset));
		
		multiInsert(CONT, 0, end, 1, wxString::Format(wxT("[(0, %u, 1), (%u, 150, 0), (151, 200, 1)]"), end, 101 + offset));
		multiInsert(SDIFF, 0, end, 1, wxString::Format(wxT("[(0, %u, 1), (%u, 150, 0), (160, 200, 1)]"), end, 101 + offset));
		multiInsert(SSAME, 0, end, 1, wxT("[(0, 150, 1), (160, 200, 1)]"));
	}
}


// Multi insert before start / touching start <-> at end / touching end / after end.
TEST(RangeMap, MultiInsert_BeforeStart_TouchingStart_AtEnd_TouchingEnd_AfterEnd)
{
	for (int end_offset = 0; end_offset < 3; ++end_offset) { // (at end, touching end, after end)
		for (int start_offset = 0; start_offset < 2; ++start_offset) { // (before start, touching start)
			int start = 98 + start_offset;
			int end   = 150 + end_offset;
				
			multiInsert(CONT, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (%u, 200, 1)]"), start, end, end + 1));
			multiInsert(SDIFF, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (160, 200, 1)]"), start, end));
			multiInsert(SSAME, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (160, 200, 1)]"), start, end));
				
			multiInsert(CONT, start, end, 1, wxString::Format(wxT("[(%u, 200, 1)]"), start));
			multiInsert(SDIFF, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (160, 200, 1)]"), start, end));
			multiInsert(SSAME, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (160, 200, 1)]"), start, end));
		}
	}
}


// Multi insert touching start <-> touching start.
TEST(RangeMap, MultiInsert_TouchingStart_TouchingStart)
{
	multiInsert(CONT, 99, 99, 0, wxT("[(99, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 99, 99, 0, wxT("[(99, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 99, 99, 0, wxT("[(99, 99, 0), (100, 150, 1), (160, 200, 1)]"));
	
	multiInsert(CONT, 99, 99, 1, wxT("[(99, 99, 1), (100, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 99, 99, 1, wxT("[(99, 99, 1), (100, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 99, 99, 1, wxT("[(99, 150, 1), (160, 200, 1)]"));
}


// Multi insert touching start / at start <-> at start / inside.
TEST(RangeMap, MultiInsert_TouchingStart_AtStart_AtStart_Inside)
{
	for (int offset_a = 0; offset_a < 2; ++offset_a) { // (touching start, at start)
		for (int offset_b = 0; offset_b < 2; ++offset_b) { // (at start, inside)
			int start = 99 + offset_a;
			int end   = 100 + offset_b;
			
			multiInsert(CONT, start, end, 0, wxString::Format(wxT("[(%u, 150, 0), (151, 200, 1)]"), start));
			multiInsert(SDIFF, start, end, 0, wxString::Format(wxT("[(%u, 150, 0), (160, 200, 1)]"), start));
			multiInsert(SSAME, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (%u, 150, 1), (160, 200, 1)]"), start, end, end + 1));
			
			multiInsert(CONT, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (%u, 150, 0), (151, 200, 1)]"), start, end, end + 1));
			multiInsert(SDIFF, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (%u, 150, 0), (160, 200, 1)]"), start, end, end + 1));
			multiInsert(SSAME, start, end, 1, wxString::Format(wxT("[(%u, 150, 1), (160, 200, 1)]"), start));
		}
	}
}


// rangeMap: Multi insert at start <-> at end / touching end / after end.
TEST(RangeMap, MultiInsert_AtStart_AtEnd_TouchingEnd_AfterEnd)
{
	for (int offset = 0; offset < 3; ++offset) { // (at end, touching end, after end)
		int start = 100;
		int end   = 150 + offset;
		
		multiInsert(CONT, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (%u, 200, 1)]"), start, end, end + 1));
		multiInsert(SDIFF, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (160, 200, 1)]"), start, end));
		multiInsert(SSAME, start, end, 0, wxString::Format(wxT("[(%u, %u, 0), (160, 200, 1)]"), start, end));
		
		multiInsert(CONT, start, end, 1, wxString::Format(wxT("[(%u, 200, 1)]"), start));
		multiInsert(SDIFF, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (160, 200, 1)]"), start, end));
		multiInsert(SSAME, start, end, 1, wxString::Format(wxT("[(%u, %u, 1), (160, 200, 1)]"), start, end));
	}
}


// Multi insert inside <-> inside.
TEST(RangeMap, MultiInsert_Inside_Inside)
{
	multiInsert(CONT, 110, 140, 0, wxT("[(100, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 110, 140, 0, wxT("[(100, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 110, 140, 0, wxT("[(100, 109, 1), (110, 140, 0), (141, 150, 1), (160, 200, 1)]"));
	
	multiInsert(CONT, 110, 140, 1, wxT("[(100, 109, 0), (110, 140, 1), (141, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 110, 140, 1, wxT("[(100, 109, 0), (110, 140, 1), (141, 150, 0), (160, 200, 1)]"));
	multiInsert(SSAME, 110, 140, 1, wxT("[(100, 150, 1), (160, 200, 1)]"));
}


// Multi insert inside / at end <-> at end / touching end / after end.
TEST(RangeMap, MultiInsert_Inside_AtEnd_AtEnd_TouchingEnd_AfterEnd)
{
	for (int offset_a = 0; offset_a < 2; ++offset_a) { // (inside, at end)
		for (int offset_b = 0; offset_b < 3; ++offset_b) { // (at end, touching end, after end)
			int start = 149 + offset_a;
			int end   = 150 + offset_b;
			
			multiInsert(CONT, start, end, 0, wxString::Format(wxT("[(100, %u, 0), (%u, 200, 1)]"), end, end + 1));
			multiInsert(SDIFF, start, end, 0, wxString::Format(wxT("[(100, %u, 0), (160, 200, 1)]"), end));
			multiInsert(SSAME, start, end, 0, wxString::Format(wxT("[(100, %u, 1), (%u, %u, 0), (160, 200, 1)]"), start - 1, start, end));
			
			multiInsert(CONT, start, end, 1, wxString::Format(wxT("[(100, %u, 0), (%u, 200, 1)]"), start - 1, start));
			multiInsert(SDIFF, start, end, 1, wxString::Format(wxT("[(100, %u, 0), (%u, %u, 1), (160, 200, 1)]"), start - 1, start, end));
			multiInsert(SSAME, start, end, 1, wxString::Format(wxT("[(100, %u, 1), (160, 200, 1)]"), end));
		}
	}
}


// Multi insert touching end <-> touching end / after end.
TEST(RangeMap, MultiInsert_TouchingEnd_TouchingEnd_AfterEnd)
{
	for (int offset = 0; offset < 2; ++offset) { // (touching end, after end)
		int start = 151;
		int end   = 151 + offset;
		
		multiInsert(CONT, start, end, 0, wxString::Format(wxT("[(100, %u, 0), (%u, 200, 1)]"), end, end + 1));
		multiInsert(SDIFF, start, end, 0, wxString::Format(wxT("[(100, %u, 0), (160, 200, 1)]"), end));
		multiInsert(SSAME, start, end, 0, wxString::Format(wxT("[(100, 150, 1), (%u, %u, 0), (160, 200, 1)]"), start, end));
		
		multiInsert(CONT, start, end, 1, wxString::Format(wxT("[(100, 150, 0), (%u, 200, 1)]"), start));
		multiInsert(SDIFF, start, end, 1, wxString::Format(wxT("[(100, 150, 0), (%u, %u, 1), (160, 200, 1)]"), start, end));
		multiInsert(SSAME, start, end, 1, wxString::Format(wxT("[(100, %u, 1), (160, 200, 1)]"), end));
	}
}


// Multi insert after end <-> after end.
TEST(RangeMap, MultiInsert_AfterEnd_AfterEnd)
{
	multiInsert(CONT, 152, 170, 0, wxT("[(100, 150, 0), (151, 151, 1), (152, 170, 0), (171, 200, 1)]"));
	multiInsert(SDIFF, 152, 170, 0, wxT("[(100, 150, 0), (152, 170, 0), (171, 200, 1)]"));
	multiInsert(SSAME, 152, 170, 0, wxT("[(100, 150, 1), (152, 170, 0), (171, 200, 1)]"));
	
	multiInsert(CONT, 152, 170, 1, wxT("[(100, 150, 0), (151, 200, 1)]"));
	multiInsert(SDIFF, 152, 170, 1, wxT("[(100, 150, 0), (152, 200, 1)]"));
	multiInsert(SSAME, 152, 170, 1, wxT("[(100, 150, 1), (152, 200, 1)]"));
}


/////////////////////////////////////////////////
// The following tests exercize the erase function.
// Since the erase function use the insert function, all
// that is needed is to test that no merging is done.

// Single erase before start <-> before start.
TEST(RangeMap, Erase_BeforeStart_BeforeStart)
{
	// Test with same and different type
	doErase(0, 90, wxT("[(100, 150, 0)]"));
}


// Single erase before start <-> touching start.
TEST(RangeMap, Erase_BeforeStart_TouchingStart)
{
	doErase(0, 99, wxT("[(100, 150, 0)]"));
}


// Single erase before start <-> at start / inside.
TEST(RangeMap, Erase_BeforeStart_AtStart_InSide)
{
	for (int offset = 0; offset < 3; ++offset) { // at start, inside, inside
		doErase(0, 100 + offset, wxString::Format(wxT("[(%u, 150, 0)]"), 100 + offset + 1));
	}
}


// Single erase before start / touching start <-> at end / touching end / after end.
TEST(RangeMap, Erase_BeforeStart_TouchingStart_AtEnd_TouchingEnd_AfterEnd)
{
	// (at end, touching end, after end)
	for (int end_offset = 0; end_offset < 3; ++end_offset) {
		// (before start, touching start)
		for (int start_offset = 0; start_offset < 2; ++start_offset) {
			doErase(98 + start_offset, 150 + end_offset, wxT("[]"));
		}
	}
}


// Single erase touching start <-> touching start.
TEST(RangeMap, Erase_TouchingStart_TouchingStart) 
{
	doErase(99, 99, wxT("[(100, 150, 0)]"));
}


// Single erase touching start / at start <-> at start / inside.
TEST(RangeMap, Erase_TouchingStart_AtStart_AtStart_Inside)
{
	for (int offset_a = 0; offset_a < 2; ++offset_a) { // (touching start, at start)
		for (int offset_b = 0; offset_b < 2; ++offset_b) { // (at start, inside)
			int start = 99 + offset_a;
			int end   = 100 + offset_b;
			
			doErase(start, end, wxString::Format(wxT("[(%u, 150, 0)]"), 100 + offset_b + 1)); 
		}
	}
}


// Single erase at start <-> at end / touching end / after end.
TEST(RangeMap, Erase_AtStart_AtEnd_TouchingEnd_AfterEnd) 
{
	// (at end, touching end, after end)
	for (int offset = 0; offset < 3; ++offset) {
		doErase(100, 150 + offset, wxT("[]"));
	}
}


// Single erase inside / at end <-> at end / touching end / after end.
TEST(RangeMap, Erase_Inside_AtEnd_AtEnd_TouchingEnd_AfterEnd)
{
	// (inside, at end)
	for (int offset_a = 0; offset_a < 2; ++offset_a) {
		// (at end, touching end, after end)
		for (int offset_b = 0; offset_b < 3; ++offset_b) {
			int start = 149 + offset_a;
			int end   = 150 + offset_b;
			
			doErase(start, end, wxString::Format(wxT("[(100, %u, 0)]"), start - 1));
		}
	}
}


// Single erase touching end <-> touching end / after end.
TEST(RangeMap, Erase_TouchingEnd_TouchingEnd_AfterEnd)
{
	// (touching end, after end)
	for (int offset = 0; offset < 2; ++offset) {
		int end = 151 + offset;
		
		doErase(151, end, wxT("[(100, 150, 0)]"));
	}
}


// Single erase after end <-> after end.
TEST(RangeMap, Erase_AfterEnd_AfterEnd)
{
	doErase(152, 170, wxT("[(100, 150, 0)]"));
}


/////////////////////////////////////////////////
// The following test exercize the CRangeSet specialization.
// 

TEST(RangeMap, RangeSet)
{
	CRangeSet set;

	set.insert(0, 10);
	set.insert(20, 30);
	set.insert(40, 50);

	ASSERT_EQUALS(wxT("[(0, 10), (20, 30), (40, 50)]"), StringFrom(set));
	ASSERT_EQUALS(3u, set.size());

	{
		CRangeSet::iterator it = set.begin();
		unsigned value = 0;
		for (; it != set.end(); ++it, value += 20) {
			ASSERT_EQUALS(value, it.keyStart());
			ASSERT_EQUALS(value + 10, it.keyEnd());
		}

		ASSERT_EQUALS(60u, value);
	}

	set.erase_range(5, 45);

	ASSERT_EQUALS(wxT("[(0, 4), (46, 50)]"), StringFrom(set));
	ASSERT_EQUALS(2u, set.size());
}

