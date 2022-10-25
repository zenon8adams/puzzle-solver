﻿#ifndef __PUZZLE_SOLVER_HPP
#define __PUZZLE_SOLVER_HPP

#include <cstddef>
#include <utility>
#include <vector>
#include <forward_list>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <fstream>
#include <random>
#include "utility.hpp"

#define RED     1
#define GREEN   1 + RED
#define YELLOW  1 + GREEN
#define BLUE    1 + YELLOW
#define MAGENTA 1 + BLUE
#define CYAN    1 + MAGENTA

class PuzzleSolver
{
	struct ProgressTracker;
public:
	
	using underlying_type = ProgressTracker;
	
	PuzzleSolver( const std::string& text, std::vector<std::string> words)
	: m_words( std::move( words))
	{
		preprocess();
		buildPuzzle( text );
	}
	PuzzleSolver( std::vector<std::string> puzzle, std::vector<std::string> words)
	: m_puzzle( std::move( puzzle)), m_words( std::move( words))
	{
		preprocess();
	}
	void solve()
	{
		solve_();
		if( m_completed.size() != m_words.size() )
		{
			m_tracker.clear();
			for( const auto& w : m_words )
				if( !m_found[ w ] )
					m_rev_words.push_back( reversed( w ) );
			for( const auto& nw : m_rev_words )
			{
				ProgressTracker rev{ nw, nw.size() - 1};
				rev.reversed = true;
				m_tracker[ nw.front() ].emplace_front( rev );
			}
			solve_();
		}
	}
	
	auto matches() const
	{
		return m_completed;
	}
	
	auto puzzle() const
	{
		return m_puzzle;
	}
	
	auto words() const
	{
		return m_words;
	}
	
	static auto next( Dir direction )
	{
		return m_dirlookup[ direction ];
	}
	
private:
	void solve_()
	{
		for( int i = 0; i < m_puzzle.size(); ++i )
		{
			for( int j = 0; j < m_puzzle[ i].size(); ++j )
			{
				auto match = m_tracker.find( m_puzzle[ i][ j] );
				if( match == m_tracker.cend() ) continue ;
				step( match->second, { i, j } );
				removeStalePath( match->second );
			}
		}
	}
	
	void removeStalePath( std::forward_list<ProgressTracker>& match )
	{
		match.remove_if( [this]( auto& elm )
		{
			return elm.invalid == true || m_found.find( elm.word ) != m_found.cend();
		});
		for( auto& [ _,v ] : m_tracker )
			v.remove_if( [this]( auto& elm ){ return m_found.find( elm.word ) != m_found.cend(); });
	}
	
	void buildPuzzle( const std::string& text )
	{
		for( std::size_t offset{ 0 }; offset < text.size(); ++offset )
			if( auto line = nextLine( text, offset ); !line.empty() )
				m_puzzle.push_back( line );
	}
	
	static std::string nextLine( const std::string& text, std::size_t& offset )
	{
		std::string line;
		for( ; offset < text.size() && text[ offset ] != '\n'; ++offset )
		{
			if( text[ offset ] == ' ' )
				continue;
			line += text[ offset ];
		}
		return line;
	}

	struct Coord
	{
		int x{ NEG_INF },
		    y{ NEG_INF };
	};

	struct ProgressTracker
	{
		std::string word;
		size_t end{ SIZE_MAX}, begin{};
		Dir dmatch{ NL };
		Coord pos, start;
		bool reversed{ false }, invalid{ false };
	};

	struct ProgressTrackerHash
	{
		std::size_t operator()( const ProgressTracker& state ) const
		{
			return std::hash<std::string>{}( state.word );
		}
	};

	friend bool operator==( const ProgressTracker& l, const ProgressTracker& r )
	{
		return l.word == r.word;
	}
	
	void step( std::forward_list<ProgressTracker>& match, Coord pos )
	{
		for( auto& m : match )
		{
			ProgressTracker next{ m };
			next.pos = pos;
			if( m.dmatch == NL )
			{
				if( m.start.x == NEG_INF )
					next.start = pos;
				else
				{
					next.dmatch = newDir( m.pos, pos );
					if( next.dmatch == NL )
						continue ;
				}
				m_tracker[ next.word[ ++next.begin]].emplace_front( next);
			}
			else if( auto nd = newDir( m.pos, pos ); nd == m.dmatch )
				m_tracker[ next.word[ ++next.begin]].emplace_front( next);

			if( m.begin == m.end)
			{
				if( !m.word.empty() && tallies( m ) )
				{
					m_completed.insert( m );
					m_found[ m.word ] = true;
				}
				m.invalid = true;   // Mark the word so it can be removed.
			}
		}
	}

	/*
	 * Confirm that the found word is matches.
	 */
	bool tallies( const ProgressTracker& tracker )
	{
		Coord clone = tracker.start;
		auto p_rows = m_puzzle.size(),
			 p_cols = m_puzzle.front().size();
		for( std::size_t i = 0; i < tracker.word.size(); ++i )
		{
			if( clone.x >= p_rows || clone.y >= p_cols || m_puzzle[ clone.x ][ clone.y ] != tracker.word[ i ] )
				return false;
			clone = m_dirlookup[ tracker.dmatch ]( clone );
		}
		return true;
	}
	
	static Dir newDir( Coord oldp, Coord newp )
	{
		Dir new_dir = NL;
		int rw[] = {  0, 0, -1, 1,  1, -1, -1, 1 },
		    cl[] = { -1, 1,  0, 0, -1,  1, -1, 1 };
		for( int i = 0; i < 8; ++i )
		{
			Coord pos{ oldp.x + cl[ i], oldp.y + rw[ i] };
			if( newp.x == pos.x && newp.y == pos.y )
			{
				new_dir = Dir(i+1);
				break;
			}
		}
		return new_dir;
	}
	
	void preprocess()
	{
		for( auto& w : m_words )
		{
			std::transform( w.cbegin(), w.cend(), w.begin(), toupper );
			m_tracker[ w.front() ].emplace_front( ProgressTracker{ w, w.size() - 1});
		}
	}

	std::vector<std::string> m_rev_words;
	std::unordered_map<char, std::forward_list<ProgressTracker>> m_tracker;
	std::vector<std::string> m_puzzle, m_words;
	std::unordered_set<ProgressTracker, ProgressTrackerHash> m_completed;
	std::unordered_map<std::string, bool> m_found;
	static std::unordered_map<Dir, std::function<Coord(Coord)>> m_dirlookup;
};

std::unordered_map<Dir, 
std::function<PuzzleSolver::Coord(PuzzleSolver::Coord)>> PuzzleSolver::m_dirlookup
{
	{ Dir::NL, []( auto pos ){ return Coord{                  }; } },
	{ Dir::NT, []( auto pos ){ return Coord{ pos.x-1, pos.y   }; } },
	{ Dir::ST, []( auto pos ){ return Coord{ pos.x+1, pos.y   }; } },
	{ Dir::ET, []( auto pos ){ return Coord{ pos.x,   pos.y+1 }; } },
	{ Dir::WT, []( auto pos ){ return Coord{ pos.x,   pos.y-1 }; } },
	{ Dir::NE, []( auto pos ){ return Coord{ pos.x-1, pos.y+1 }; } },
	{ Dir::SE, []( auto pos ){ return Coord{ pos.x+1, pos.y+1 }; } },
	{ Dir::NW, []( auto pos ){ return Coord{ pos.x-1, pos.y-1 }; } },
	{ Dir::SW, []( auto pos ){ return Coord{ pos.x+1, pos.y-1 }; } }
};

class PuzzleFileReader
{
public:
	explicit PuzzleFileReader( std::istream& strm )
	: m_istrm( strm )
	{
		ignoreBOM();
	}
	PuzzleFileReader operator=( const PuzzleFileReader& ) = delete;
	PuzzleFileReader( const PuzzleFileReader& )           = delete;
	auto getPuzzles()
	{
		parseFile();
		std::call_once( m_parse_flag, [this]{  parseFile(); } );
		return m_puzzles;
	}
private:
	
	void ignoreBOM()
	{
		if( m_istrm.peek() == 0xEF )
		{
			char dummy[ 4];
			m_istrm.get( dummy, 4 );
		}
	}
	
	void parseFile()
	{
		ParseMode mode = NILL;
		std::vector<std::string> cur[ 2];
		while( m_istrm )
		{
			auto w = nextWord();
			if( w.empty() )
				continue;
			if( mode != NILL && w.back() != ':' )
				cur[ mode-1 ].push_back( shaped( w ) );
			else
			{
				if( mode != NILL && !cur[ 0].empty() && !cur[ 1].empty() )
				{
					m_puzzles.push_back( {} );
					m_puzzles.back().puzzle = cur[ 0];
					m_puzzles.back().keys   = cur[ 1];
					cur[ 0].clear(); cur[ 1].clear();
				}
				
				if( strcasecmp( w.data(), "puzzle:" )  == 0 )
					mode = PUZZLE;
				else if( strcasecmp( w.data(), "key:" ) == 0 )
					mode = KEY;
				else
					mode = NILL;
			}
		}
		
	}
	
	static std::string shaped( const std::string& given )
	{
		std::string new_s;
		std::copy_if( given.cbegin(), given.cend(),
		              std::back_inserter( new_s), isalpha );
		return new_s;
	}
	
	std::string nextWord()
	{
		std::string word;
		std::decay_t<decltype( m_istrm)>::char_type c;
		
		while( m_istrm.get( c ) && c != ' ' && c != '\n' )
			word += c;
		return word;
	}

	enum ParseMode
	{
		NILL,
		PUZZLE,
		KEY
	};
	struct PuzzleImage
	{
		std::vector<std::string> puzzle,
		                         keys;
	};
	std::vector<PuzzleImage> m_puzzles;
	std::istream& m_istrm;
	std::once_flag m_parse_flag;
};

#endif
