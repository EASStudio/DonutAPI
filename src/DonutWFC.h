#pragma once

#ifndef DONUTWFC_H
#define DONUTWFC_H

#define DONUTWFC_VERSION "1.1"

// Module includes
#include "DonutAPI.h"

// Core includes
#include <cstdint>
#include <climits>
#include <cmath>
#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace WFC
{
	enum class Direction : uint8_t
	{
		North,
		East,
		South,
		West,
		NorthEast,
		NorthWest,
		SouthEast,
		SouthWest,
		Count
	};

	struct DirectionHash
	{
		size_t operator()(Direction d) const noexcept
		{
			return static_cast<size_t>(static_cast<uint8_t>(d));
		}
	};

	inline Direction GetOppositeDirection(Direction dir) noexcept
	{
		switch (dir)
		{
		case Direction::North:     return Direction::South;
		case Direction::South:     return Direction::North;
		case Direction::East:      return Direction::West;
		case Direction::West:      return Direction::East;
		case Direction::NorthEast: return Direction::SouthWest;
		case Direction::NorthWest: return Direction::SouthEast;
		case Direction::SouthEast: return Direction::NorthWest;
		case Direction::SouthWest: return Direction::NorthEast;
		default:                   return dir;
		}
	}

	inline Vector2 GetDirectionOffset(Direction dir) noexcept
	{
		switch (dir)
		{
		case Direction::North:     return { 0.0f, -1.0f };
		case Direction::South:     return { 0.0f,  1.0f };
		case Direction::East:      return { 1.0f,  0.0f };
		case Direction::West:      return { -1.0f, 0.0f };
		case Direction::NorthEast: return { 1.0f, -1.0f };
		case Direction::NorthWest: return { -1.0f, -1.0f };
		case Direction::SouthEast: return { 1.0f,  1.0f };
		case Direction::SouthWest: return { -1.0f,  1.0f };
		default:                   return { 0.0f,  0.0f };
		}
	}

	inline const char* GetDirectionName(Direction dir) noexcept
	{
		switch (dir)
		{
		case Direction::North:     return "North";
		case Direction::South:     return "South";
		case Direction::East:      return "East";
		case Direction::West:      return "West";
		case Direction::NorthEast: return "NorthEast";
		case Direction::NorthWest: return "NorthWest";
		case Direction::SouthEast: return "SouthEast";
		case Direction::SouthWest: return "SouthWest";
		default:                   return "Unknown";
		}
	}

	inline bool IsCardinalDirection(Direction dir) noexcept
	{
		return dir == Direction::North || dir == Direction::South ||
			dir == Direction::East || dir == Direction::West;
	}

	inline std::vector<Direction> GetCardinalDirections() noexcept
	{
		return { Direction::North, Direction::South, Direction::East, Direction::West };
	}

	inline std::vector<Direction> GetAllDirections() noexcept
	{
		return { Direction::North, Direction::South, Direction::East, Direction::West,
				 Direction::NorthEast, Direction::NorthWest, Direction::SouthEast, Direction::SouthWest };
	}

	// Tile::color is a COLOR attribute (0-15), but DisplayMap()/TileColor() expect an
	// ANSI escape *string*. This converts between the two so the WFC renderers can
	// feed DonutAPI's string-map renderer directly.
	inline const std::string& ColorToAnsi(unsigned short color) noexcept
	{
		switch (color & 0x000F)
		{
		case BLACK:         return TILE_BLACK;
		case DARK_BLUE:     return TILE_DARK_BLUE;
		case DARK_GREEN:    return TILE_DARK_GREEN;
		case DARK_CYAN:     return TILE_DARK_CYAN;
		case DARK_RED:      return TILE_DARK_RED;
		case DARK_MAGENTA:  return TILE_DARK_MAGENTA;
		case BROWN:         return TILE_BROWN;
		case GRAY:          return TILE_GRAY;
		case DARK_GRAY:     return TILE_DARK_GRAY;
		case BLUE:          return TILE_BLUE;
		case GREEN:         return TILE_GREEN;
		case CYAN:          return TILE_CYAN;
		case RED:           return TILE_RED;
		case MAGENTA:       return TILE_MAGENTA;
		case YELLOW:        return TILE_YELLOW;
		default:            return TILE_WHITE;
		}
	}

	enum class EntropyMode : uint8_t
	{
		Simple,
		Shannon
	};

	enum class ContradictionHandling : uint8_t
	{
		Restart,
		Backtrack,
		RandomRepair
	};

	enum class VisualizationMode : uint8_t
	{
		Instant,
		Animated,
		StepByStep,
		Paused,
		SingleStep
	};

	enum class PresetType : uint8_t
	{
		Terrain,
		Dungeon,
		Village,
		Cave,
		Island,
		City,
		Maze
	};

	struct TileMetadata
	{
		std::string biome = "none";
		float elevation = 0.0f;
		float temperature = 0.5f;
		float moisture = 0.5f;
		std::string category = "none";
		float movementCost = 1.0f;
		int priority = 0;
	};

	// Declared before PropagationRecord / DecisionPoint, both of which store these.
	struct CellSnapshot
	{
		int cellIndex = -1;
		std::vector<int> oldPossibilities;
		int oldSelectedTile = -1;
		bool wasCollapsed = false;
		float oldEntropy = 0.0f;
	};

	struct PropagationRecord
	{
		int sourceCellIndex = -1;
		std::vector<CellSnapshot> changes;
	};

	struct DecisionPoint
	{
		int cellIndex = -1;
		std::vector<int> triedTiles;
		std::vector<CellSnapshot> propagatedChanges;
	};

	struct GenerationStats
	{
		int iterations = 0;
		int propagations = 0;
		int contradictions = 0;
		int backtracks = 0;
		int cellsCollapsed = 0;
		int totalCells = 0;
		double generationTimeMs = 0.0;
		double avgEntropy = 0.0;
		int minEntropy = 0;
		int maxEntropy = 0;
		int currentQueueSize = 0;
	};

	struct WFCConfig
	{
		unsigned int seed = 42;
		EntropyMode entropyMode = EntropyMode::Shannon;
		ContradictionHandling contradictionHandling = ContradictionHandling::Restart;
		VisualizationMode visualizationMode = VisualizationMode::Instant;
		int animationDelayMs = 15;
		bool debugLogging = false;
		bool debugOverlay = false;
		int maxBacktrackDepth = 100;
		int maxIterations = 100000;
		bool useDiagonalRules = false;
		float restartAttempts = 3.0f;
	};

	class Tile
	{
	public:
		int id = -1;
		char symbol = '?';
		std::string name = "Unknown";
		unsigned short color = GRAY;
		float weight = 1.0f;
		bool walkable = true;
		bool transparent = true;
		bool blocksMovement = false;
		bool blocksVision = false;
		TileDef donutTile = {};

		TileMetadata metadata;

		Texture texture = {};
		bool hasTexture = false;

		std::unordered_set<int> north;
		std::unordered_set<int> south;
		std::unordered_set<int> east;
		std::unordered_set<int> west;

		std::unordered_set<int> northEast;
		std::unordered_set<int> northWest;
		std::unordered_set<int> southEast;
		std::unordered_set<int> southWest;

		std::unordered_set<std::string> tags;

		bool canRotate = false;
		bool canReflect = false;
		int rotationGroup = -1;

		Tile() = default;

		Tile(int id, char symbol, const std::string& name, unsigned short color, float weight)
			: id(id), symbol(symbol), name(name), color(color), weight(weight)
		{

		}

		Tile(int id, char symbol, const std::string& name, unsigned short color, float weight, const Texture& tex)
			: id(id), symbol(symbol), name(name), color(color), weight(weight), texture(tex), hasTexture(true)
		{

		}

		std::unordered_set<int>& GetAllowedNeighbors(Direction dir) noexcept;
		const std::unordered_set<int>& GetAllowedNeighbors(Direction dir) const noexcept;
		void AddNeighbor(Direction dir, int tileId);
		bool HasTag(const std::string& tag) const noexcept;
		void AddTag(const std::string& tag);

		// Texture
		void SetTexture(const Texture& tex);
		const Texture& GetTexture() const;
		bool HasTexture() const noexcept { return hasTexture; }
	};

	class Cell
	{
	public:
		Vector2 position = { 0.0f, 0.0f };
		bool collapsed = false;
		int selectedTile = -1;
		float entropy = 0.0f;
		std::vector<int> possibilities;

		Cell() = default;

		explicit Cell(Vector2 pos) : position(pos) {}

		void Reset(const std::vector<int>& allTileIds);
		void Collapse(int tileId) noexcept;
		bool IsCollapsed() const noexcept { return collapsed; }
		bool IsContradiction() const noexcept { return possibilities.empty() && !collapsed; }
		float CalculateEntropy(const std::vector<Tile>& tiles, EntropyMode mode) const;
		bool RemovePossibility(int tileId);
		bool HasPossibility(int tileId) const noexcept;
		size_t GetPossibilityCount() const noexcept { return possibilities.size(); }
	};

	class Grid
	{
	public:
		Grid() = default;

		void Initialize(int w, int h, const std::vector<int>& tileIds);
		void Reset(const std::vector<int>& tileIds);
		void Resize(int w, int h, const std::vector<int>& tileIds);
		Cell& GetCell(int x, int y);
		const Cell& GetCell(int x, int y) const;
		Cell& GetCellByIndex(int index);
		const Cell& GetCellByIndex(int index) const;
		std::vector<std::pair<Cell*, Direction>> GetNeighbors(int x, int y, bool includeDiagonals = false);
		std::vector<std::pair<const Cell*, Direction>> GetNeighbors(int x, int y, bool includeDiagonals = false) const;
		int GetIndex(int x, int y) const noexcept;
		Vector2 GetPosition(int index) const noexcept;
		bool InsideBounds(int x, int y) const noexcept;
		int GetWidth() const noexcept { return width; }
		int GetHeight() const noexcept { return height; }
		size_t GetCellCount() const noexcept { return cells.size(); }
		std::vector<Cell>& GetCells() noexcept { return cells; }
		const std::vector<Cell>& GetCells() const noexcept { return cells; }
		size_t CountCollapsed() const;
		size_t CountContradictions() const;
		bool IsFullyCollapsed() const;
		bool HasContradiction() const;

	private:
		int width = 0;
		int height = 0;
		std::vector<Cell> cells;
	};

	class RuleSet
	{
	public:
		RuleSet() = default;

		int AddTile(Tile tile);
		bool RemoveTile(int id);
		void AddNeighborRule(int fromTile, Direction dir, int toTile);
		void AddBidirectionalRule(int tileA, Direction dir, int tileB);
		bool CanConnect(int fromTile, Direction dir, int toTile) const;
		bool ValidateRules() const;
		void Clear();
		const Tile* GetTile(int id) const;
		Tile* GetTileMutable(int id);
		const std::vector<Tile>& GetTiles() const noexcept { return tiles; }
		size_t GetTileCount() const noexcept { return tiles.size(); }
		std::vector<int> GetTileIds() const;
		float GetTotalWeight() const;
		float GetTileWeight(int id) const;
		const Tile* FindTileBySymbol(char symbol) const;
		const Tile* FindTileByName(const std::string& name) const;
		std::vector<const Tile*> GetTilesWithTag(const std::string& tag) const;

		// Presets - each one clears the rule set first and rebuilds it from scratch
		void LoadTerrainPreset();
		void LoadDungeonPreset();
		void LoadVillagePreset();
		void LoadCavePreset();
		void LoadIslandPreset();
		void LoadCityPreset();
		void LoadMazePreset();
		void LoadPreset(PresetType preset);

		// Texture
		void SetTileTexture(int tileId, const Texture& tex);
		bool LoadTileTexture(int tileId, const std::wstring& path);

	private:
		std::vector<Tile> tiles;
		std::unordered_map<int, size_t> idToIndex;
		int nextId = 0;
	};

	class WaveFunctionCollapse
	{
	public:
		WaveFunctionCollapse();

		explicit WaveFunctionCollapse(const WFCConfig& cfg);

		~WaveFunctionCollapse() = default;

		void SetSeed(unsigned int seed);

		unsigned int GetSeed() const noexcept { return config.seed; }

		void SetEntropyMode(EntropyMode mode);

		void SetContradictionHandling(ContradictionHandling mode);

		void SetVisualizationMode(VisualizationMode mode);

		void SetDebugLogging(bool enabled);

		void SetDebugOverlay(bool enabled);

		void SetAnimationDelay(int ms);

		void SetMaxBacktrackDepth(int depth);

		void SetMaxIterations(int maxIter);

		void SetUseDiagonalRules(bool enabled);

		void SetRestartAttempts(float attempts);

		const WFCConfig& GetConfig() const noexcept { return config; }

		void CreateGrid(int width, int height);

		void ResizeGrid(int width, int height);

		Grid& GetGrid() { return grid; }

		const Grid& GetGrid() const { return grid; }

		RuleSet& GetRuleSet() { return rules; }

		const RuleSet& GetRuleSet() const { return rules; }

		void LoadTerrainPreset();

		void LoadDungeonPreset();

		void LoadVillagePreset();

		void LoadCavePreset();

		void LoadIslandPreset();

		void LoadCityPreset();

		void LoadMazePreset();

		void LoadPreset(PresetType preset);

		void ClearRules();

		void SetBorderConstraint(Direction dir, int tileId);

		void SetPreCollapsedCell(int x, int y, int tileId);

		void ClearConstraints();

		void AddRegionConstraint(int x, int y, int w, int h, const std::string& requiredTag);

		bool Generate();

		bool Step();

		void Reset();

		void FullReset();

		bool IsSolved() const noexcept { return solved; }

		bool IsFailed() const noexcept { return failed; }

		bool IsGenerating() const noexcept { return generating; }

		bool IsInitialized() const noexcept { return initialized; }

		void Render() const;

		void RenderEntropy() const;

		void RenderPossibilities() const;

		void AnimateGeneration();

		void RenderDebugOverlay();

		void DrawCell(int x, int y) const;

		std::vector<std::vector<char>> ExportCharMap() const;

		std::vector<std::vector<int>> ExportTileIDs() const;

		void ExportToDonutMap();

		const Tile* GetTileAt(int x, int y) const;

		const GenerationStats& GetStats() const noexcept { return stats; }

		int GetIterations() const noexcept { return stats.iterations; }

		int GetPropagationCount() const noexcept { return stats.propagations; }

		int GetContradictions() const noexcept { return stats.contradictions; }

		int GetBacktracks() const noexcept { return stats.backtracks; }

		double GetGenerationTime() const noexcept { return stats.generationTimeMs; }

		std::vector<int> GenerateRotations(int baseTileId);

		int GenerateReflection(int baseTileId, bool horizontal);

		bool ExtractRulesFromMap(const std::vector<std::vector<char>>& charMap, int tileSize = 2);

		void SetupChunkGeneration(int chunkWidth, int chunkHeight, int overlapSize = 1);

		bool GenerateChunk(int chunkX, int chunkY, const std::vector<std::pair<Vector2, int>>& borderTiles = {});

		void ApplyDistanceFalloff(float centerX, float centerY, float maxDistance, int tileId, float weightMult);

		void ConstrainRegion(int x, int y, int w, int h, const std::vector<std::string>& allowedTags);

		// Texture
		void RenderWithTextures(int screenX, int screenY, int tileSize = 8) const;

		void DrawTexturedMap(int offsetX = 0, int offsetY = 0, int tileSize = 8) const;

	private:
		// config is declared first on purpose: rng is seeded from config.seed in the
		// constructor initialiser list, and members initialise in declaration order.
		WFCConfig config;
		Grid grid;
		RuleSet rules;
		std::mt19937 rng;
		GenerationStats stats;

		bool solved = false;
		bool failed = false;
		bool initialized = false;
		bool generating = false;

		std::unordered_map<Direction, int, DirectionHash> borderConstraints;
		std::unordered_map<int, int> preCollapsedCells;

		std::vector<DecisionPoint> decisionStack;
		int currentBacktrackDepth = 0;

		std::chrono::high_resolution_clock::time_point startTime;

		int chunkWidth = 0;
		int chunkHeight = 0;
		int overlapSize = 0;
		bool useChunkGeneration = false;

		void InitializeRNG();
		Cell* FindLowestEntropyCell();
		bool CollapseCell(Cell& cell);
		void Propagate(Cell& cell, std::vector<CellSnapshot>& changes);
		bool PropagateStep(int cellIndex, std::vector<CellSnapshot>& changes);
		bool HandleContradiction();
		void ApplyBorderConstraints();
		void ApplyPreCollapsedCells();
		void UpdateEntropy();
		void SaveCellState(int cellIndex, std::vector<CellSnapshot>& changes);
		void RestoreCellState(const CellSnapshot& snapshot);
		void RecordDecision(int cellIndex, int chosenTile, const std::vector<CellSnapshot>& changes);
		bool UndoLastDecision();
		void ClearDecisionStack();

		// Shared by Propagate()/PropagateStep(): may `toTile` sit in direction `dir`
		// from `fromTile`? Both halves of the rule have to agree.
		bool Compatible(int fromTile, Direction dir, int toTile) const;
	};

}

#endif // DONUTWFC_H