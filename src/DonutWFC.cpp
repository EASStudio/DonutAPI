// Module includes
#include "DonutWFC.h"

// Core includes
#include <queue>
#include <climits>
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>

namespace WFC
{
    // Wave Function collapse functions (Top of the file because these are the ones tha matter the most)
    WaveFunctionCollapse::WaveFunctionCollapse()
        : config(), rng(config.seed)
    {
        InternalLog("WFC: WaveFunctionCollapse initialized");
    }

    WaveFunctionCollapse::WaveFunctionCollapse(const WFCConfig& cfg)
        : config(cfg), rng(cfg.seed)
    {
        InternalLog("WFC: WaveFunctionCollapse initialized with custom config");
    }

    void WaveFunctionCollapse::SetSeed(unsigned int seed)
    {
        config.seed = seed;
        InitializeRNG();

        if (config.debugLogging)
        {
            std::ostringstream oss;
            oss << "WFC: Seed set to " << seed;
            InternalLog(oss.str().c_str());
        }
    }

    void WaveFunctionCollapse::SetEntropyMode(EntropyMode mode)
    {
        config.entropyMode = mode;
    }

    void WaveFunctionCollapse::SetContradictionHandling(ContradictionHandling mode)
    {
        config.contradictionHandling = mode;
    }

    void WaveFunctionCollapse::SetVisualizationMode(VisualizationMode mode)
    {
        config.visualizationMode = mode;
    }

    void WaveFunctionCollapse::SetDebugLogging(bool enabled)
    {
        config.debugLogging = enabled;
    }

    void WaveFunctionCollapse::SetDebugOverlay(bool enabled)
    {
        config.debugOverlay = enabled;
    }

    void WaveFunctionCollapse::SetAnimationDelay(int ms)
    {
        config.animationDelayMs = std::max(0, ms);
    }

    void WaveFunctionCollapse::SetMaxBacktrackDepth(int depth)
    {
        config.maxBacktrackDepth = std::max(0, depth);
    }

    void WaveFunctionCollapse::SetMaxIterations(int maxIter)
    {
        config.maxIterations = std::max(1, maxIter);
    }

    void WaveFunctionCollapse::SetUseDiagonalRules(bool enabled)
    {
        config.useDiagonalRules = enabled;
    }

    void WaveFunctionCollapse::SetRestartAttempts(float attempts)
    {
        config.restartAttempts = std::max(1.0f, attempts);
    }

    void WaveFunctionCollapse::InitializeRNG()
    {
        rng.seed(config.seed);
    }

    void WaveFunctionCollapse::CreateGrid(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            InternalLog("WFC: Invalid grid dimensions");
            return;
        }

        grid.Initialize(width, height, rules.GetTileIds());
        initialized = true;
        solved = false;
        failed = false;
        generating = false;
        ClearDecisionStack();

        if (config.debugLogging)
        {
            std::ostringstream oss;
            oss << "WFC: Created grid " << width << "x" << height
                << " with " << rules.GetTileCount() << " tiles";
            InternalLog(oss.str().c_str());
        }
    }

    void WaveFunctionCollapse::ResizeGrid(int width, int height)
    {
        CreateGrid(width, height);
    }

    // These used to call themselves, which was infinite recursion; the tile tables
    // they meant to reach now live on RuleSet, so each one forwards there and then
    // re-seeds the grid with the new tile ids.
    void WaveFunctionCollapse::LoadTerrainPreset()
    {
        rules.LoadTerrainPreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadDungeonPreset()
    {
        rules.LoadDungeonPreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadVillagePreset()
    {
        rules.LoadVillagePreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadCavePreset()
    {
        rules.LoadCavePreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadIslandPreset()
    {
        rules.LoadIslandPreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadCityPreset()
    {
        rules.LoadCityPreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadMazePreset()
    {
        rules.LoadMazePreset();
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::LoadPreset(PresetType preset)
    {
        rules.LoadPreset(preset);
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
        }
    }

    void WaveFunctionCollapse::ClearRules()
    {
        rules.Clear();
    }

    void WaveFunctionCollapse::SetBorderConstraint(Direction dir, int tileId)
    {
        borderConstraints[dir] = tileId;

        if (config.debugLogging)
        {
            std::ostringstream oss;
            oss << "WFC: Border constraint set: " << GetDirectionName(dir)
                << " -> tile " << tileId;
            InternalLog(oss.str().c_str());
        }
    }

    void WaveFunctionCollapse::SetPreCollapsedCell(int x, int y, int tileId)
    {
        if (!grid.InsideBounds(x, y))
        {
            return;
        }

        int index = grid.GetIndex(x, y);
        preCollapsedCells[index] = tileId;

        if (config.debugLogging)
        {
            std::ostringstream oss;
            oss << "WFC: Pre-collapsed cell at (" << x << "," << y
                << ") -> tile " << tileId;
            InternalLog(oss.str().c_str());
        }
    }

    void WaveFunctionCollapse::ClearConstraints()
    {
        borderConstraints.clear();
        preCollapsedCells.clear();
    }

    void WaveFunctionCollapse::AddRegionConstraint(int x, int y, int w, int h,
        const std::string& requiredTag)
    {
        if (!initialized)
        {
            return;
        }

        auto taggedTiles = rules.GetTilesWithTag(requiredTag);
        if (taggedTiles.empty())
        {
            return;
        }

        std::vector<int> allowedIds;
        allowedIds.reserve(taggedTiles.size());
        for (const auto* tile : taggedTiles)
        {
            allowedIds.push_back(tile->id);
        }

        for (int ry = y; ry < y + h && ry < grid.GetHeight(); ++ry)
        {
            for (int rx = x; rx < x + w && rx < grid.GetWidth(); ++rx)
            {
                if (grid.InsideBounds(rx, ry))
                {
                    Cell& cell = grid.GetCell(rx, ry);
                    std::vector<int> newPossibilities;
                    for (int id : cell.possibilities)
                    {
                        if (std::find(allowedIds.begin(), allowedIds.end(), id) != allowedIds.end())
                        {
                            newPossibilities.push_back(id);
                        }
                    }
                    cell.possibilities = std::move(newPossibilities);
                }
            }
        }
    }

    void WaveFunctionCollapse::ApplyBorderConstraints()
    {
        int w = grid.GetWidth();
        int h = grid.GetHeight();

        for (const auto& [dir, tileId] : borderConstraints)
        {
            for (int i = 0; i < w; ++i)
            {
                for (int j = 0; j < h; ++j)
                {
                    bool isBorder = false;

                    switch (dir)
                    {
                    case Direction::North: isBorder = (j == 0); break;
                    case Direction::South: isBorder = (j == h - 1); break;
                    case Direction::East:  isBorder = (i == w - 1); break;
                    case Direction::West:  isBorder = (i == 0); break;
                    default: break;
                    }

                    if (isBorder)
                    {
                        Cell& cell = grid.GetCell(i, j);
                        cell.possibilities.clear();
                        cell.possibilities.push_back(tileId);
                        cell.collapsed = true;
                        cell.selectedTile = tileId;
                        cell.entropy = 0.0f;
                    }
                }
            }
        }
    }

    void WaveFunctionCollapse::ApplyPreCollapsedCells()
    {
        for (const auto& [index, tileId] : preCollapsedCells)
        {
            Cell& cell = grid.GetCellByIndex(index);
            cell.Collapse(tileId);
        }
    }

    void WaveFunctionCollapse::UpdateEntropy()
    {
        float totalEntropy = 0.0f;
        int minE = INT_MAX;
        int maxE = 0;
        int count = 0;

        for (auto& cell : grid.GetCells())
        {
            if (!cell.collapsed)
            {
                float e = cell.CalculateEntropy(rules.GetTiles(), config.entropyMode);
                cell.entropy = e;
                totalEntropy += e;
                int eInt = static_cast<int>(cell.GetPossibilityCount());
                minE = std::min(minE, eInt);
                maxE = std::max(maxE, eInt);
                ++count;
            }
        }

        stats.avgEntropy = count > 0 ? totalEntropy / count : 0.0f;
        stats.minEntropy = minE == INT_MAX ? 0 : minE;
        stats.maxEntropy = maxE;
    }

    Cell* WaveFunctionCollapse::FindLowestEntropyCell()
    {
        Cell* bestCell = nullptr;
        float bestEntropy = std::numeric_limits<float>::max();

        std::uniform_real_distribution<float> noiseDist(0.0f, 0.001f);

        for (auto& cell : grid.GetCells())
        {
            if (cell.collapsed || cell.possibilities.empty())
            {
                continue;
            }

            float entropy = cell.entropy + noiseDist(rng);

            if (entropy < bestEntropy)
            {
                bestEntropy = entropy;
                bestCell = &cell;
            }
        }

        return bestCell;
    }

    bool WaveFunctionCollapse::CollapseCell(Cell& cell)
    {
        if (cell.possibilities.empty())
        {
            return false;
        }

        std::vector<float> weights;
        weights.reserve(cell.possibilities.size());

        for (int tileId : cell.possibilities)
        {
            weights.push_back(rules.GetTileWeight(tileId));
        }

        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        int selectedIndex = dist(rng);
        int chosenTile = cell.possibilities[selectedIndex];

        cell.Collapse(chosenTile);

        if (config.debugLogging)
        {
            const Tile* tile = rules.GetTile(chosenTile);
            std::ostringstream oss;
            oss << "WFC: Collapsed (" << static_cast<int>(cell.position.x)
                << "," << static_cast<int>(cell.position.y)
                << ") to " << (tile ? tile->name : "unknown")
                << " (entropy was " << cell.entropy << ")";
            DebugLog(oss.str().c_str());
        }

        return true;
    }

    void WaveFunctionCollapse::SaveCellState(int cellIndex, std::vector<CellSnapshot>& changes)
    {
        const Cell& cell = grid.GetCellByIndex(cellIndex);

        for (const auto& snap : changes)
        {
            if (snap.cellIndex == cellIndex)
            {
                return;
            }
        }

        CellSnapshot snapshot;
        snapshot.cellIndex = cellIndex;
        snapshot.oldPossibilities = cell.possibilities;
        snapshot.oldSelectedTile = cell.selectedTile;
        snapshot.wasCollapsed = cell.collapsed;
        snapshot.oldEntropy = cell.entropy;

        changes.push_back(std::move(snapshot));
    }

    void WaveFunctionCollapse::RestoreCellState(const CellSnapshot& snapshot)
    {
        Cell& cell = grid.GetCellByIndex(snapshot.cellIndex);
        cell.possibilities = snapshot.oldPossibilities;
        cell.selectedTile = snapshot.oldSelectedTile;
        cell.collapsed = snapshot.wasCollapsed;
        cell.entropy = snapshot.oldEntropy;
    }

    // `dir` always points FROM the cell being propagated TO its neighbour, so the
    // rule to consult is fromTile's set for `dir` and toTile's set for the opposite
    // direction. The old code looked those two up the wrong way round and OR'd them,
    // which let illegal neighbours survive propagation.
    bool WaveFunctionCollapse::Compatible(int fromTile, Direction dir, int toTile) const
    {
        return rules.CanConnect(fromTile, dir, toTile) &&
            rules.CanConnect(toTile, GetOppositeDirection(dir), fromTile);
    }

    void WaveFunctionCollapse::Propagate(Cell& sourceCell, std::vector<CellSnapshot>& changes)
    {
        std::queue<int> propagationQueue;

        int sourceIndex = grid.GetIndex(
            static_cast<int>(sourceCell.position.x),
            static_cast<int>(sourceCell.position.y)
        );
        propagationQueue.push(sourceIndex);

        while (!propagationQueue.empty())
        {
            int currentIndex = propagationQueue.front();
            propagationQueue.pop();

            const Cell& currentCell = grid.GetCellByIndex(currentIndex);
            Vector2 pos = currentCell.position;
            int x = static_cast<int>(pos.x);
            int y = static_cast<int>(pos.y);

            auto neighbors = grid.GetNeighbors(x, y, config.useDiagonalRules);

            for (auto& [neighbor, dir] : neighbors)
            {
                if (neighbor->collapsed)
                {
                    continue;
                }

                std::vector<int> newPossibilities;
                newPossibilities.reserve(neighbor->possibilities.size());

                for (int tileId : neighbor->possibilities)
                {
                    bool valid = false;

                    for (int currentTileId : currentCell.possibilities)
                    {
                        if (Compatible(currentTileId, dir, tileId))
                        {
                            valid = true;
                            break;
                        }
                    }

                    if (valid)
                    {
                        newPossibilities.push_back(tileId);
                    }
                }

                if (newPossibilities.size() != neighbor->possibilities.size())
                {
                    int neighborIndex = grid.GetIndex(
                        static_cast<int>(neighbor->position.x),
                        static_cast<int>(neighbor->position.y)
                    );

                    SaveCellState(neighborIndex, changes);

                    neighbor->possibilities = std::move(newPossibilities);
                    neighbor->entropy = neighbor->CalculateEntropy(
                        rules.GetTiles(), config.entropyMode
                    );

                    ++stats.propagations;

                    if (!neighbor->possibilities.empty())
                    {
                        propagationQueue.push(neighborIndex);
                    }
                }
            }
        }
    }

    bool WaveFunctionCollapse::PropagateStep(int cellIndex, std::vector<CellSnapshot>& changes)
    {
        const Cell& currentCell = grid.GetCellByIndex(cellIndex);
        Vector2 pos = currentCell.position;
        int x = static_cast<int>(pos.x);
        int y = static_cast<int>(pos.y);

        bool anyChanged = false;

        auto neighbors = grid.GetNeighbors(x, y, config.useDiagonalRules);

        for (auto& [neighbor, dir] : neighbors)
        {
            if (neighbor->collapsed)
            {
                continue;
            }

            std::vector<int> newPossibilities;
            newPossibilities.reserve(neighbor->possibilities.size());

            for (int tileId : neighbor->possibilities)
            {
                bool valid = false;

                for (int currentTileId : currentCell.possibilities)
                {
                    if (Compatible(currentTileId, dir, tileId))
                    {
                        valid = true;
                        break;
                    }
                }

                if (valid)
                {
                    newPossibilities.push_back(tileId);
                }
            }

            if (newPossibilities.size() != neighbor->possibilities.size())
            {
                int neighborIndex = grid.GetIndex(
                    static_cast<int>(neighbor->position.x),
                    static_cast<int>(neighbor->position.y)
                );

                SaveCellState(neighborIndex, changes);

                neighbor->possibilities = std::move(newPossibilities);
                neighbor->entropy = neighbor->CalculateEntropy(
                    rules.GetTiles(), config.entropyMode
                );

                ++stats.propagations;
                anyChanged = true;
            }
        }

        return anyChanged;
    }

    void WaveFunctionCollapse::RecordDecision(int cellIndex, int chosenTile,
        const std::vector<CellSnapshot>& changes)
    {
        DecisionPoint dp;
        dp.cellIndex = cellIndex;
        dp.triedTiles.push_back(chosenTile);
        dp.propagatedChanges = changes;
        decisionStack.push_back(std::move(dp));
        // currentBacktrackDepth counts *consecutive undos*, not stack size - bumping
        // it here made maxBacktrackDepth an upper bound on total collapses instead.
    }

    bool WaveFunctionCollapse::UndoLastDecision()
    {
        if (decisionStack.empty())
        {
            return false;
        }

        DecisionPoint& dp = decisionStack.back();

        for (const auto& snapshot : dp.propagatedChanges)
        {
            RestoreCellState(snapshot);
        }

        Cell& cell = grid.GetCellByIndex(dp.cellIndex);
        cell.collapsed = false;
        cell.selectedTile = -1;

        std::vector<int> untried;
        for (int tileId : cell.possibilities)
        {
            if (std::find(dp.triedTiles.begin(), dp.triedTiles.end(), tileId) == dp.triedTiles.end())
            {
                untried.push_back(tileId);
            }
        }

        if (untried.empty())
        {
            decisionStack.pop_back();
            ++stats.backtracks;
            return UndoLastDecision();
        }

        // Re-snapshot the restored source cell so a later undo of this same decision
        // can still see its full possibility list.
        dp.propagatedChanges.clear();
        SaveCellState(dp.cellIndex, dp.propagatedChanges);

        cell.Collapse(untried[0]);
        dp.triedTiles.push_back(untried[0]);

        Propagate(cell, dp.propagatedChanges);

        ++stats.backtracks;
        ++currentBacktrackDepth;

        if (config.debugLogging)
        {
            std::ostringstream oss;
            oss << "WFC: Backtracked to cell " << dp.cellIndex
                << ", trying tile " << untried[0];
            InternalLog(oss.str().c_str());
        }

        return true;
    }

    void WaveFunctionCollapse::ClearDecisionStack()
    {
        decisionStack.clear();
        currentBacktrackDepth = 0;
    }

    bool WaveFunctionCollapse::HandleContradiction()
    {
        ++stats.contradictions;

        if (config.debugLogging)
        {
            InternalLog("WFC: Contradiction detected!");
        }

        switch (config.contradictionHandling)
        {
        case ContradictionHandling::Restart:
            return false;

        case ContradictionHandling::Backtrack:
            if (currentBacktrackDepth >= config.maxBacktrackDepth || decisionStack.empty())
            {
                return false;
            }
            return UndoLastDecision();

        case ContradictionHandling::RandomRepair:
        {
            for (auto& cell : grid.GetCells())
            {
                if (cell.IsContradiction())
                {
                    std::vector<int> validTiles;
                    Vector2 pos = cell.position;
                    int x = static_cast<int>(pos.x);
                    int y = static_cast<int>(pos.y);

                    auto neighbors = grid.GetNeighbors(x, y, config.useDiagonalRules);

                    for (const auto& tile : rules.GetTiles())
                    {
                        bool valid = true;
                        for (const auto& [neighbor, dir] : neighbors)
                        {
                            if (neighbor->collapsed)
                            {
                                Direction oppDir = GetOppositeDirection(dir);
                                if (!rules.CanConnect(tile.id, dir, neighbor->selectedTile) &&
                                    !rules.CanConnect(neighbor->selectedTile, oppDir, tile.id))
                                {
                                    valid = false;
                                    break;
                                }
                            }
                        }
                        if (valid)
                        {
                            validTiles.push_back(tile.id);
                        }
                    }

                    if (!validTiles.empty())
                    {
                        std::uniform_int_distribution<int> dist(0,
                            static_cast<int>(validTiles.size()) - 1);
                        int chosen = validTiles[dist(rng)];
                        cell.Collapse(chosen);

                        std::vector<CellSnapshot> dummy;
                        Propagate(cell, dummy);

                        return true;
                    }
                }
            }
            return false;
        }

        default:
            return false;
        }
    }

    bool WaveFunctionCollapse::Generate()
    {
        if (!initialized || rules.GetTileCount() == 0)
        {
            InternalLog("WFC: Cannot generate - not initialized or no rules");
            return false;
        }

        startTime = std::chrono::high_resolution_clock::now();

        solved = false;
        failed = false;
        generating = true;
        stats = GenerationStats{};
        stats.totalCells = static_cast<int>(grid.GetCellCount());
        ClearDecisionStack();

        grid.Reset(rules.GetTileIds());

        ApplyBorderConstraints();
        ApplyPreCollapsedCells();

        UpdateEntropy();

        if (config.debugLogging)
        {
            InternalLog("WFC: Starting generation");
        }

        int attempts = 0;
        float maxAttempts = config.restartAttempts;

        while (attempts < static_cast<int>(maxAttempts) && !solved)
        {
            if (attempts > 0)
            {
                grid.Reset(rules.GetTileIds());
                ApplyBorderConstraints();
                ApplyPreCollapsedCells();
                UpdateEntropy();
                ClearDecisionStack();

                if (config.debugLogging)
                {
                    std::ostringstream oss;
                    oss << "WFC: Restart attempt " << (attempts + 1);
                    InternalLog(oss.str().c_str());
                }
            }

            bool contradiction = false;

            while (!grid.IsFullyCollapsed() && !contradiction)
            {
                if (stats.iterations >= config.maxIterations)
                {
                    contradiction = true;
                    break;
                }

                if (grid.HasContradiction())
                {
                    if (!HandleContradiction())
                    {
                        contradiction = true;
                        break;
                    }
                    continue;
                }

                UpdateEntropy();

                Cell* cell = FindLowestEntropyCell();
                if (!cell)
                {
                    break;
                }

                int cellIndex = grid.GetIndex(
                    static_cast<int>(cell->position.x),
                    static_cast<int>(cell->position.y)
                );

                // Snapshot the cell BEFORE collapsing. The old code recorded the
                // post-collapse state as the "old" state, so backtracking restored a
                // single-possibility cell and instantly gave up.
                std::vector<CellSnapshot> changes;
                SaveCellState(cellIndex, changes);

                if (!CollapseCell(*cell))
                {
                    contradiction = true;
                    break;
                }

                Propagate(*cell, changes);

                RecordDecision(cellIndex, cell->selectedTile, changes);
                currentBacktrackDepth = 0;

                ++stats.iterations;
                ++stats.cellsCollapsed;

                if (config.visualizationMode == VisualizationMode::Animated)
                {
                    Render();
                    if (config.debugOverlay)
                    {
                        RenderDebugOverlay();
                    }
                    Delay(config.animationDelayMs);
                }
            }

            if (!contradiction && grid.IsFullyCollapsed())
            {
                solved = true;
            }

            ++attempts;
        }

        generating = false;

        auto endTime = std::chrono::high_resolution_clock::now();
        stats.generationTimeMs = std::chrono::duration<double, std::milli>(
            endTime - startTime).count();

        if (solved)
        {
            if (config.debugLogging)
            {
                std::ostringstream oss;
                oss << "WFC: Generation complete in " << stats.iterations
                    << " iterations, " << stats.generationTimeMs << "ms";
                InternalLog(oss.str().c_str());
            }
        }
        else
        {
            failed = true;
            InternalLog("WFC: Generation failed");
        }

        return solved;
    }

    bool WaveFunctionCollapse::Step()
    {
        if (!initialized || rules.GetTileCount() == 0)
        {
            return false;
        }

        if (!generating)
        {
            startTime = std::chrono::high_resolution_clock::now();
            solved = false;
            failed = false;
            generating = true;
            stats = GenerationStats{};
            stats.totalCells = static_cast<int>(grid.GetCellCount());

            grid.Reset(rules.GetTileIds());
            ApplyBorderConstraints();
            ApplyPreCollapsedCells();
            UpdateEntropy();
        }

        if (solved || failed)
        {
            return false;
        }

        if (grid.HasContradiction())
        {
            if (!HandleContradiction())
            {
                failed = true;
                generating = false;
                return false;
            }
            return true;
        }

        UpdateEntropy();

        Cell* cell = FindLowestEntropyCell();
        if (!cell)
        {
            if (grid.IsFullyCollapsed())
            {
                solved = true;
            }
            else
            {
                failed = true;
            }
            generating = false;
            return false;
        }

        int cellIndex = grid.GetIndex(
            static_cast<int>(cell->position.x),
            static_cast<int>(cell->position.y)
        );

        std::vector<CellSnapshot> changes;
        SaveCellState(cellIndex, changes);

        if (!CollapseCell(*cell))
        {
            failed = true;
            generating = false;
            return false;
        }

        Propagate(*cell, changes);
        RecordDecision(cellIndex, cell->selectedTile, changes);
        currentBacktrackDepth = 0;

        ++stats.iterations;
        ++stats.cellsCollapsed;

        if (grid.IsFullyCollapsed())
        {
            solved = true;
            generating = false;

            auto endTime = std::chrono::high_resolution_clock::now();
            stats.generationTimeMs = std::chrono::duration<double, std::milli>(
                endTime - startTime).count();
        }

        return true;
    }

    void WaveFunctionCollapse::Reset()
    {
        if (initialized)
        {
            grid.Reset(rules.GetTileIds());
            ApplyBorderConstraints();
            ApplyPreCollapsedCells();
        }

        solved = false;
        failed = false;
        generating = false;
        stats = GenerationStats{};
        ClearDecisionStack();
    }

    void WaveFunctionCollapse::FullReset()
    {
        grid = Grid();
        initialized = false;
        solved = false;
        failed = false;
        generating = false;
        stats = GenerationStats{};
        ClearConstraints();
        ClearDecisionStack();
    }

    void WaveFunctionCollapse::Render() const
    {
        if (!initialized)
        {
            return;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        std::vector<std::vector<char>> charMap(h, std::vector<char>(w, '?'));
        std::unordered_map<char, std::string> tileColors;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);

                if (cell.collapsed)
                {
                    const Tile* tile = rules.GetTile(cell.selectedTile);
                    if (tile)
                    {
                        charMap[y][x] = tile->symbol;
                        // DisplayMap() wants ANSI escapes, not raw COLOR attributes
                        tileColors[tile->symbol] = ColorToAnsi(tile->color);
                    }
                }
                else if (cell.possibilities.empty())
                {
                    charMap[y][x] = 'X';
                    tileColors['X'] = ColorToAnsi(COLOR::RED);
                }
                else
                {
                    charMap[y][x] = ' ';
                    tileColors[' '] = ColorToAnsi(COLOR::GRAY);
                }
            }
        }

        SetMapVector(charMap);
        DisplayMap(h, 1, 1, tileColors);
    }

    void WaveFunctionCollapse::RenderEntropy() const
    {
        if (!initialized)
        {
            return;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        std::vector<std::vector<char>> charMap(h, std::vector<char>(w, ' '));
        std::unordered_map<char, std::string> tileColors;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);

                if (cell.collapsed)
                {
                    charMap[y][x] = '#';
                    tileColors['#'] = ColorToAnsi(COLOR::DARK_GREEN);
                }
                else
                {
                    int count = static_cast<int>(cell.GetPossibilityCount());
                    if (count <= 9)
                    {
                        charMap[y][x] = '0' + count;
                    }
                    else
                    {
                        charMap[y][x] = '+';
                    }

                    const size_t tileTotal = rules.GetTileCount();
                    float normalizedEntropy = tileTotal > 0
                        ? static_cast<float>(count) / static_cast<float>(tileTotal)
                        : 0.0f;

                    if (normalizedEntropy < 0.3f)
                    {
                        tileColors[charMap[y][x]] = ColorToAnsi(COLOR::GREEN);
                    }
                    else if (normalizedEntropy < 0.6f)
                    {
                        tileColors[charMap[y][x]] = ColorToAnsi(COLOR::YELLOW);
                    }
                    else
                    {
                        tileColors[charMap[y][x]] = ColorToAnsi(COLOR::RED);
                    }
                }
            }
        }

        SetMapVector(charMap);
        DisplayMap(h, 1, 1, tileColors);
    }

    void WaveFunctionCollapse::RenderPossibilities() const
    {
        if (!initialized)
        {
            return;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        std::vector<std::vector<char>> charMap(h, std::vector<char>(w, ' '));
        std::unordered_map<char, std::string> tileColors;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);

                if (cell.collapsed)
                {
                    const Tile* tile = rules.GetTile(cell.selectedTile);
                    charMap[y][x] = tile ? tile->symbol : '?';
                    if (tile)
                    {
                        tileColors[tile->symbol] = ColorToAnsi(tile->color);
                    }
                }
                else
                {
                    size_t count = cell.GetPossibilityCount();
                    if (count == 0)
                    {
                        charMap[y][x] = 'X';
                        tileColors['X'] = ColorToAnsi(COLOR::RED);
                    }
                    else if (count == 1)
                    {
                        charMap[y][x] = '.';
                        tileColors['.'] = ColorToAnsi(COLOR::GREEN);
                    }
                    else if (count <= 3)
                    {
                        charMap[y][x] = ':';
                        tileColors[':'] = ColorToAnsi(COLOR::YELLOW);
                    }
                    else if (count <= 6)
                    {
                        charMap[y][x] = '=';
                        tileColors['='] = ColorToAnsi(COLOR::CYAN);
                    }
                    else
                    {
                        charMap[y][x] = '~';
                        tileColors['~'] = ColorToAnsi(COLOR::BLUE);
                    }
                }
            }
        }

        SetMapVector(charMap);
        DisplayMap(h, 1, 1, tileColors);
    }

    void WaveFunctionCollapse::AnimateGeneration()
    {
        VisualizationMode oldMode = config.visualizationMode;
        config.visualizationMode = VisualizationMode::Animated;

        Generate();

        config.visualizationMode = oldMode;
    }

    void WaveFunctionCollapse::RenderDebugOverlay()
    {
        if (!config.debugOverlay)
            return;

        int queueSize = 0;
        for (const auto& cell : grid.GetCells())
        {
            if (!cell.collapsed && !cell.possibilities.empty())
                ++queueSize;
        }

        stats.currentQueueSize = queueSize;

        std::ostringstream oss;
        oss << "\n=== WFC Debug Overlay ===\n"
            << "Seed: " << config.seed << "\n"
            << "Grid: " << grid.GetWidth() << "x" << grid.GetHeight() << "\n"
            << "Tiles: " << rules.GetTileCount() << "\n"
            << "Solved: " << (solved ? "Yes" : "No") << "\n"
            << "Failed: " << (failed ? "Yes" : "No") << "\n"
            << "Iterations: " << stats.iterations << "\n"
            << "Propagations: " << stats.propagations << "\n"
            << "Contradictions: " << stats.contradictions << "\n"
            << "Backtracks: " << stats.backtracks << "\n"
            << "Cells Collapsed: " << stats.cellsCollapsed << "/" << stats.totalCells << "\n"
            << "Remaining: " << queueSize << "\n"
            << "Avg Entropy: " << std::fixed << std::setprecision(2) << stats.avgEntropy << "\n"
            << "Min/Max Possibilities: " << stats.minEntropy << "/" << stats.maxEntropy << "\n"
            << "Backtrack Depth: " << currentBacktrackDepth << "\n"
            << "Time: " << std::fixed << std::setprecision(1) << stats.generationTimeMs << "ms\n"
            << "Entropy Mode: " << (config.entropyMode == EntropyMode::Simple ? "Simple" : "Shannon") << "\n"
            << "Contradiction Handling: ";

        switch (config.contradictionHandling)
        {
        case ContradictionHandling::Restart: oss << "Restart"; break;
        case ContradictionHandling::Backtrack: oss << "Backtrack"; break;
        case ContradictionHandling::RandomRepair: oss << "RandomRepair"; break;
        }

        oss << "\n========================\n";

        InternalLog(oss.str().c_str());
    }

    void WaveFunctionCollapse::DrawCell(int x, int y) const
    {
        if (!grid.InsideBounds(x, y))
        {
            return;
        }

        const Cell& cell = grid.GetCell(x, y);

        if (cell.collapsed)
        {
            const Tile* tile = rules.GetTile(cell.selectedTile);
            if (tile)
            {
                if (config.debugLogging)
                {
                    std::ostringstream oss;
                    oss << "Cell(" << x << "," << y << "): " << tile->name
                        << " [" << tile->symbol << "]";
                    DebugLog(oss.str().c_str());
                }
            }
        }
    }

    std::vector<std::vector<char>> WaveFunctionCollapse::ExportCharMap() const
    {
        std::vector<std::vector<char>> result;

        if (!initialized)
        {
            return result;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        result.resize(h, std::vector<char>(w, '?'));

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);

                if (cell.collapsed)
                {
                    const Tile* tile = rules.GetTile(cell.selectedTile);
                    result[y][x] = tile ? tile->symbol : '?';
                }
                else
                {
                    result[y][x] = ' ';
                }
            }
        }

        return result;
    }

    std::vector<std::vector<int>> WaveFunctionCollapse::ExportTileIDs() const
    {
        std::vector<std::vector<int>> result;

        if (!initialized)
        {
            return result;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        result.resize(h, std::vector<int>(w, -1));

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);
                result[y][x] = cell.selectedTile;
            }
        }

        return result;
    }

    void WaveFunctionCollapse::ExportToDonutMap()
    {
        if (!initialized)
        {
            return;
        }

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        std::vector<std::vector<char>> charMap(h, std::vector<char>(w, ' '));
        std::unordered_map<char, std::string> tileColors;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);

                if (cell.collapsed)
                {
                    Tile* tile = rules.GetTileMutable(cell.selectedTile);
                    if (tile)
                    {
                        charMap[y][x] = tile->symbol;
                        tileColors[tile->symbol] = ColorToAnsi(tile->color);
                        tile->donutTile.tile = tile->symbol;
                        tile->donutTile.weight = static_cast<int>(tile->weight);
                    }
                }
            }
        }

        SetMapVector(charMap);
        DisplayMap(h, 1, 1, tileColors);

        if (config.debugLogging)
        {
            InternalLog("WFC: Exported map to DonutAPI");
        }
    }

    const Tile* WaveFunctionCollapse::GetTileAt(int x, int y) const
    {
        if (!grid.InsideBounds(x, y))
        {
            return nullptr;
        }

        const Cell& cell = grid.GetCell(x, y);
        if (!cell.collapsed)
        {
            return nullptr;
        }

        return rules.GetTile(cell.selectedTile);
    }

    std::vector<int> WaveFunctionCollapse::GenerateRotations(int baseTileId)
    {
        std::vector<int> newIds;

        const Tile* base = rules.GetTile(baseTileId);
        if (!base || !base->canRotate)
        {
            return newIds;
        }

        auto rotateDir = [](Direction d) -> Direction {
            switch (d)
            {
            case Direction::North: return Direction::East;
            case Direction::East:  return Direction::South;
            case Direction::South: return Direction::West;
            case Direction::West:  return Direction::North;
            default: return d;
            }
            };

        // Copy by value first: AddTile() below can reallocate the rule set's tile
        // vector, which would leave `base` dangling on the second iteration.
        const Tile baseTile = *base;

        auto rotateDirBy = [&rotateDir](Direction d, int turns)
            {
                for (int i = 0; i < turns; ++i) d = rotateDir(d);
                return d;
            };

        for (int rot = 0; rot < 3; ++rot)
        {
            // 90, 180 and 270 degrees - the old code applied a single 90 degree turn
            // every time, producing three identical copies.
            const int turns = rot + 1;

            Tile rotated = baseTile;
            rotated.id = -1;

            rotated.name = baseTile.name + "_rot" + std::to_string(turns);
            rotated.rotationGroup = baseTile.rotationGroup >= 0 ? baseTile.rotationGroup : baseTile.id;

            rotated.north.clear();
            rotated.south.clear();
            rotated.east.clear();
            rotated.west.clear();

            for (int n : baseTile.north) rotated.GetAllowedNeighbors(rotateDirBy(Direction::North, turns)).insert(n);
            for (int n : baseTile.east)  rotated.GetAllowedNeighbors(rotateDirBy(Direction::East, turns)).insert(n);
            for (int n : baseTile.south) rotated.GetAllowedNeighbors(rotateDirBy(Direction::South, turns)).insert(n);
            for (int n : baseTile.west)  rotated.GetAllowedNeighbors(rotateDirBy(Direction::West, turns)).insert(n);

            int newId = rules.AddTile(std::move(rotated));
            newIds.push_back(newId);
        }

        return newIds;
    }

    int WaveFunctionCollapse::GenerateReflection(int baseTileId, bool horizontal)
    {
        const Tile* base = rules.GetTile(baseTileId);
        if (!base || !base->canReflect)
        {
            return -1;
        }

        Tile reflected = *base;
        reflected.id = -1;
        reflected.name = base->name + (horizontal ? "_refH" : "_refV");

        reflected.north.clear();
        reflected.south.clear();
        reflected.east.clear();
        reflected.west.clear();

        if (horizontal)
        {
            reflected.north = base->north;
            reflected.south = base->south;
            reflected.east = base->west;
            reflected.west = base->east;
        }
        else
        {
            reflected.north = base->south;
            reflected.south = base->north;
            reflected.east = base->east;
            reflected.west = base->west;
        }

        return rules.AddTile(std::move(reflected));
    }

    bool WaveFunctionCollapse::ExtractRulesFromMap(const std::vector<std::vector<char>>& charMap,
        int tileSize)
    {
        if (charMap.empty() || charMap[0].empty())
        {
            return false;
        }

        (void)tileSize; // reserved for NxN pattern extraction

        // Start from a clean slate; otherwise a second call stacks new ids on top of
        // the old tiles and every char maps to the wrong tile.
        rules.Clear();

        std::unordered_set<char> uniqueChars;
        for (const auto& row : charMap)
        {
            for (char c : row)
            {
                if (c != ' ')
                {
                    uniqueChars.insert(c);
                }
            }
        }

        std::unordered_map<char, int> charToId;
        int id = 0;
        for (char c : uniqueChars)
        {
            Tile t(id, c, std::string(1, c), COLOR::WHITE, 1.0f);
            rules.AddTile(std::move(t));
            charToId[c] = id++;
        }

        int h = static_cast<int>(charMap.size());
        int w = static_cast<int>(charMap[0].size());

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                char c = charMap[y][x];
                if (c == ' ') continue;

                int fromId = charToId[c];

                if (x > 0 && charMap[y][x - 1] != ' ')
                {
                    int toId = charToId[charMap[y][x - 1]];
                    rules.AddBidirectionalRule(fromId, Direction::West, toId);
                }

                if (x < w - 1 && charMap[y][x + 1] != ' ')
                {
                    int toId = charToId[charMap[y][x + 1]];
                    rules.AddBidirectionalRule(fromId, Direction::East, toId);
                }

                if (y > 0 && charMap[y - 1][x] != ' ')
                {
                    int toId = charToId[charMap[y - 1][x]];
                    rules.AddBidirectionalRule(fromId, Direction::North, toId);
                }

                if (y < h - 1 && charMap[y + 1][x] != ' ')
                {
                    int toId = charToId[charMap[y + 1][x]];
                    rules.AddBidirectionalRule(fromId, Direction::South, toId);
                }
            }
        }

        return true;
    }

    void WaveFunctionCollapse::SetupChunkGeneration(int cWidth, int cHeight, int overlap)
    {
        chunkWidth = cWidth;
        chunkHeight = cHeight;
        overlapSize = overlap;
        useChunkGeneration = true;
    }

    bool WaveFunctionCollapse::GenerateChunk(int chunkX, int chunkY,
        const std::vector<std::pair<Vector2, int>>& borderTiles)
    {
        if (!useChunkGeneration)
        {
            return false;
        }

        // chunkX / chunkY are the caller's coordinates for the chunk being built;
        // the generator itself works in local grid space.
        (void)chunkX;
        (void)chunkY;

        int totalWidth = chunkWidth + overlapSize * 2;
        int totalHeight = chunkHeight + overlapSize * 2;

        CreateGrid(totalWidth, totalHeight);

        for (const auto& [pos, tileId] : borderTiles)
        {
            int x = static_cast<int>(pos.x);
            int y = static_cast<int>(pos.y);
            if (grid.InsideBounds(x, y))
            {
                SetPreCollapsedCell(x, y, tileId);
            }
        }

        return Generate();
    }

    void WaveFunctionCollapse::ApplyDistanceFalloff(float centerX, float centerY,
        float maxDistance, int tileId,
        float weightMult)
    {
        Tile* tile = rules.GetTileMutable(tileId);
        if (!tile || !initialized || maxDistance <= 0.0f)
        {
            return;
        }

        // The old body computed `falloff` and threw it away, so the call did nothing.
        // Cells keep tileId with probability falloff * weightMult: certain at the
        // centre, impossible at maxDistance and beyond.
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);

        for (auto& cell : grid.GetCells())
        {
            if (cell.collapsed) continue;

            float dist = Vector2Distance(cell.position, { centerX, centerY });
            float falloff = (dist < maxDistance) ? (1.0f - (dist / maxDistance)) : 0.0f;
            float keepChance = Clamp(falloff * weightMult, 0.0f, 1.0f);

            if (roll(rng) > keepChance)
            {
                cell.RemovePossibility(tileId);
                cell.entropy = cell.CalculateEntropy(rules.GetTiles(), config.entropyMode);
            }
        }
    }

    void WaveFunctionCollapse::ConstrainRegion(int x, int y, int w, int h,
        const std::vector<std::string>& allowedTags)
    {
        if (!initialized)
        {
            return;
        }

        std::vector<int> allowedIds;

        for (const auto& tile : rules.GetTiles())
        {
            for (const auto& tag : allowedTags)
            {
                if (tile.HasTag(tag))
                {
                    allowedIds.push_back(tile.id);
                    break;
                }
            }
        }

        if (allowedIds.empty())
        {
            return;
        }

        for (int ry = y; ry < y + h && ry < grid.GetHeight(); ++ry)
        {
            for (int rx = x; rx < x + w && rx < grid.GetWidth(); ++rx)
            {
                if (grid.InsideBounds(rx, ry))
                {
                    Cell& cell = grid.GetCell(rx, ry);
                    std::vector<int> newPoss;

                    for (int id : cell.possibilities)
                    {
                        if (std::find(allowedIds.begin(), allowedIds.end(), id) != allowedIds.end())
                        {
                            newPoss.push_back(id);
                        }
                    }

                    cell.possibilities = std::move(newPoss);
                    cell.entropy = cell.CalculateEntropy(rules.GetTiles(), config.entropyMode);
                }
            }
        }
    }

    void RuleSet::LoadTerrainPreset()
    {
        Clear();

        AddTile(Tile(0, '.', "Grass", COLOR::YELLOW, 25.0f));
        AddTile(Tile(1, 'F', "Forest", COLOR::DARK_GREEN, 15.0f));
        AddTile(Tile(2, 'T', "Tree", COLOR::GREEN, 12.0f));
        AddTile(Tile(3, '"', "Bush", COLOR::GREEN, 8.0f));
        AddTile(Tile(4, '~', "Water", COLOR::BLUE, 10.0f));
        AddTile(Tile(5, static_cast<char>(0xB0), "DeepWater", COLOR::DARK_BLUE, 5.0f));
        AddTile(Tile(6, '=', "River", COLOR::CYAN, 6.0f));
        AddTile(Tile(7, ':', "Sand", COLOR::BROWN, 8.0f));
        AddTile(Tile(8, '#', "Stone", COLOR::GRAY, 6.0f));
        AddTile(Tile(9, '^', "Mountain", COLOR::DARK_GRAY, 4.0f));
        AddTile(Tile(10, '*', "Snow", COLOR::WHITE, 3.0f));
        AddTile(Tile(11, 'L', "Lava", COLOR::RED, 1.0f));
        AddTile(Tile(12, 'R', "Road", COLOR::BROWN, 10.0f));
        AddTile(Tile(13, 'B', "Bridge", COLOR::YELLOW, 2.0f));
        AddTile(Tile(14, 'V', "Village", COLOR::YELLOW, 3.0f));
        AddTile(Tile(15, 'C', "Castle", COLOR::CYAN, 1.0f));

        // Tiles 16-21 are referenced by the tag / metadata / adjacency tables below
        // but were never registered, so every rule naming them was silently dropped
        // and the "22 tiles" log line was wrong.
        AddTile(Tile(16, 'O', "Cave", COLOR::DARK_GRAY, 2.0f));
        AddTile(Tile(17, 's', "Swamp", COLOR::DARK_GREEN, 4.0f));
        AddTile(Tile(18, 'f', "Flowers", COLOR::MAGENTA, 5.0f));
        AddTile(Tile(19, 'P', "Plaza", COLOR::GRAY, 4.0f));
        AddTile(Tile(20, 'W', "Wall", COLOR::DARK_GRAY, 4.0f));
        AddTile(Tile(21, 'D', "Door", COLOR::BROWN, 2.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(4, false, true, true, false);
        setProps(5, false, true, true, false);
        setProps(6, false, true, true, false);
        setProps(9, false, true, true, false);
        setProps(10, true, true, false, false);
        setProps(11, false, true, true, false);
        setProps(15, false, false, true, true);
        setProps(16, false, false, true, true);
        setProps(17, false, true, true, false);
        setProps(20, false, false, true, true);
        setProps(21, true, false, false, true);

        auto addTag = [this](int id, const std::string& tag) {
            Tile* t = GetTileMutable(id);
            if (t) t->AddTag(tag);
            };

        addTag(0, "ground"); addTag(0, "natural");
        addTag(1, "vegetation"); addTag(1, "natural"); addTag(1, "forest");
        addTag(2, "vegetation"); addTag(2, "natural"); addTag(2, "tree");
        addTag(3, "vegetation"); addTag(3, "natural");
        addTag(4, "water"); addTag(4, "natural");
        addTag(5, "water"); addTag(5, "natural"); addTag(5, "deep");
        addTag(6, "water"); addTag(6, "natural"); addTag(6, "river");
        addTag(7, "ground"); addTag(7, "natural"); addTag(7, "beach");
        addTag(8, "stone"); addTag(8, "natural");
        addTag(9, "mountain"); addTag(9, "natural"); addTag(9, "elevated");
        addTag(10, "snow"); addTag(10, "natural"); addTag(10, "elevated");
        addTag(11, "lava"); addTag(11, "hazard"); addTag(11, "natural");
        addTag(12, "road"); addTag(12, "structure");
        addTag(13, "road"); addTag(13, "structure"); addTag(13, "bridge");
        addTag(14, "building"); addTag(14, "structure"); addTag(14, "civilized");
        addTag(15, "building"); addTag(15, "structure"); addTag(15, "fortified");
        addTag(16, "building"); addTag(16, "structure"); addTag(16, "underground");
        addTag(17, "water"); addTag(17, "natural"); addTag(17, "swamp");
        addTag(18, "vegetation"); addTag(18, "natural");
        addTag(19, "ground"); addTag(19, "structure"); addTag(19, "civilized");
        addTag(20, "wall"); addTag(20, "structure");
        addTag(21, "door"); addTag(21, "structure");

        auto setMeta = [this](int id, const std::string& biome, float elev, float temp, float moist) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->metadata.biome = biome;
                t->metadata.elevation = elev;
                t->metadata.temperature = temp;
                t->metadata.moisture = moist;
            }
            };

        setMeta(0, "plains", 0.0f, 0.6f, 0.5f);
        setMeta(1, "forest", 0.1f, 0.5f, 0.7f);
        setMeta(2, "forest", 0.1f, 0.5f, 0.6f);
        setMeta(3, "plains", 0.0f, 0.6f, 0.5f);
        setMeta(4, "water", -0.5f, 0.4f, 1.0f);
        setMeta(5, "ocean", -1.0f, 0.3f, 1.0f);
        setMeta(6, "river", -0.3f, 0.4f, 1.0f);
        setMeta(7, "beach", -0.1f, 0.7f, 0.3f);
        setMeta(8, "mountain", 0.5f, 0.3f, 0.2f);
        setMeta(9, "mountain", 0.8f, 0.2f, 0.1f);
        setMeta(10, "snow", 1.0f, 0.0f, 0.1f);
        setMeta(11, "volcanic", 0.3f, 1.0f, 0.0f);
        setMeta(12, "plains", 0.0f, 0.6f, 0.4f);
        setMeta(13, "river", -0.3f, 0.4f, 1.0f);
        setMeta(14, "plains", 0.0f, 0.6f, 0.4f);
        setMeta(15, "mountain", 0.4f, 0.3f, 0.2f);
        setMeta(16, "cave", -0.5f, 0.5f, 0.3f);
        setMeta(17, "swamp", -0.2f, 0.5f, 0.9f);
        setMeta(18, "plains", 0.0f, 0.6f, 0.5f);
        setMeta(19, "plains", 0.0f, 0.6f, 0.6f);
        setMeta(20, "mountain", 0.5f, 0.3f, 0.2f);
        setMeta(21, "mountain", 0.5f, 0.3f, 0.2f);

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0);
        bi(0, 1);
        bi(0, 2);
        bi(0, 3);
        bi(0, 7);
        bi(0, 12);
        bi(0, 14);
        bi(0, 18);
        bi(0, 19);

        bi(1, 1);
        bi(1, 2);
        bi(1, 3);
        bi(1, 9);
        bi(1, 17);

        bi(2, 2);
        bi(2, 3);

        bi(3, 3);
        bi(3, 18);

        bi(4, 4);
        bi(4, 5);
        bi(4, 7);
        bi(4, 6);
        bi(4, 17);
        bi(4, 13);

        bi(5, 5);

        bi(6, 6);
        bi(6, 13);
        bi(6, 17);

        bi(7, 7);
        bi(7, 12);

        bi(8, 8);
        bi(8, 9);
        bi(8, 10);
        bi(8, 11);
        bi(8, 12);
        bi(8, 20);

        bi(9, 9);
        bi(9, 10);
        bi(9, 11);
        bi(9, 15);
        bi(9, 20);

        bi(10, 10);

        bi(11, 11);

        bi(12, 12);
        bi(12, 13);
        bi(12, 14);
        bi(12, 15);
        bi(12, 19);
        bi(12, 21);

        bi(13, 13);

        bi(14, 14);
        bi(14, 19);

        bi(15, 15);
        bi(15, 20);
        bi(15, 21);

        bi(16, 16);
        bi(16, 20);
        bi(16, 21);

        bi(17, 17);

        bi(18, 18);

        bi(19, 19);

        bi(20, 20);
        bi(20, 21);

        bi(21, 21);

        InternalLog("WFC: Loaded terrain preset with 22 tiles");
    }

    void RuleSet::LoadDungeonPreset()
    {
        Clear();

        AddTile(Tile(0, '#', "Wall", COLOR::GRAY, 40.0f));
        AddTile(Tile(1, '.', "Floor", COLOR::DARK_GRAY, 35.0f));
        AddTile(Tile(2, 'H', "Door", COLOR::BROWN, 5.0f));
        AddTile(Tile(3, '>', "StairsDown", COLOR::YELLOW, 2.0f));
        AddTile(Tile(4, '<', "StairsUp", COLOR::YELLOW, 2.0f));
        AddTile(Tile(5, '+', "Chest", COLOR::YELLOW, 2.0f));
        AddTile(Tile(6, 'T', "Trap", COLOR::RED, 3.0f));
        AddTile(Tile(7, '~', "Water", COLOR::BLUE, 3.0f));
        AddTile(Tile(8, '*', "Rubble", COLOR::DARK_GRAY, 5.0f));
        AddTile(Tile(9, ' ', "Void", COLOR::BLACK, 0.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(0, false, false, true, true);
        setProps(2, true, false, false, true);
        setProps(7, false, true, true, false);
        setProps(9, false, true, true, true);

        auto addTag = [this](int id, const std::string& tag) {
            Tile* t = GetTileMutable(id);
            if (t) t->AddTag(tag);
            };

        addTag(0, "wall"); addTag(0, "solid");
        addTag(1, "floor"); addTag(1, "walkable");
        addTag(2, "door"); addTag(2, "walkable");
        addTag(3, "stairs"); addTag(3, "walkable"); addTag(3, "exit");
        addTag(4, "stairs"); addTag(4, "walkable"); addTag(4, "entrance");
        addTag(5, "item"); addTag(5, "walkable");
        addTag(6, "hazard"); addTag(6, "walkable");
        addTag(7, "water"); addTag(7, "hazard");
        addTag(8, "obstacle"); addTag(8, "walkable");
        addTag(9, "void"); addTag(9, "solid");

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1); bi(0, 2); bi(0, 3); bi(0, 4); bi(0, 5); bi(0, 6); bi(0, 7); bi(0, 8);

        bi(1, 1); bi(1, 2); bi(1, 3); bi(1, 4); bi(1, 5); bi(1, 6); bi(1, 7); bi(1, 8);

        bi(2, 2); bi(2, 1);

        bi(3, 1); bi(3, 3);
        bi(4, 1); bi(4, 4);

        bi(5, 1); bi(5, 5);

        bi(6, 1); bi(6, 6);

        bi(7, 7); bi(7, 0);

        bi(8, 1); bi(8, 8); bi(8, 0);

        bi(9, 9); bi(9, 0);

        InternalLog("WFC: Loaded dungeon preset with 10 tiles");
    }

    void RuleSet::LoadVillagePreset()
    {
        Clear();

        AddTile(Tile(0, '.', "Path", COLOR::BROWN, 20.0f));
        AddTile(Tile(1, 'G', "Grass", COLOR::GREEN, 15.0f));
        AddTile(Tile(2, 'H', "House", COLOR::YELLOW, 10.0f));
        AddTile(Tile(3, 'W', "Wall", COLOR::GRAY, 8.0f));
        AddTile(Tile(4, 'D', "Door", COLOR::BROWN, 5.0f));
        AddTile(Tile(5, 'T', "Tree", COLOR::GREEN, 10.0f));
        AddTile(Tile(6, 'F', "Farm", COLOR::DARK_GREEN, 8.0f));
        AddTile(Tile(7, 'w', "Well", COLOR::CYAN, 2.0f));
        AddTile(Tile(8, 'S', "Shop", COLOR::YELLOW, 4.0f));
        AddTile(Tile(9, 'C', "Church", COLOR::WHITE, 2.0f));
        AddTile(Tile(10, '~', "Pond", COLOR::BLUE, 3.0f));
        AddTile(Tile(11, 'f', "Fence", COLOR::YELLOW, 6.0f));
        AddTile(Tile(12, 'M', "Market", COLOR::MAGENTA, 3.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(2, false, false, true, true);
        setProps(3, false, false, true, true);
        setProps(7, false, true, true, false);
        setProps(9, false, false, true, true);
        setProps(10, false, true, true, false);
        setProps(11, false, true, true, false);

        auto addTag = [this](int id, const std::string& tag) {
            Tile* t = GetTileMutable(id);
            if (t) t->AddTag(tag);
            };

        addTag(0, "path"); addTag(0, "walkable");
        addTag(1, "ground"); addTag(1, "natural"); addTag(1, "walkable");
        addTag(2, "building"); addTag(2, "structure");
        addTag(3, "wall"); addTag(3, "structure");
        addTag(4, "door"); addTag(4, "structure"); addTag(4, "walkable");
        addTag(5, "tree"); addTag(5, "natural"); addTag(5, "walkable");
        addTag(6, "farm"); addTag(6, "structure"); addTag(6, "walkable");
        addTag(7, "water"); addTag(7, "structure");
        addTag(8, "shop"); addTag(8, "building"); addTag(8, "structure");
        addTag(9, "church"); addTag(9, "building"); addTag(9, "structure");
        addTag(10, "water"); addTag(10, "natural");
        addTag(11, "fence"); addTag(11, "structure");
        addTag(12, "market"); addTag(12, "building"); addTag(12, "structure"); addTag(12, "walkable");

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1); bi(0, 4); bi(0, 7); bi(0, 12);
        bi(1, 1); bi(1, 5); bi(1, 6); bi(1, 10); bi(1, 11);
        bi(2, 2); bi(2, 3); bi(2, 4);
        bi(3, 3); bi(3, 4); bi(3, 11);
        bi(4, 4); bi(4, 0);
        bi(5, 5);
        bi(6, 6); bi(6, 11);
        bi(7, 7); bi(7, 0);
        bi(8, 8); bi(8, 3); bi(8, 4); bi(8, 0);
        bi(9, 9); bi(9, 3); bi(9, 4); bi(9, 0);
        bi(10, 10);
        bi(11, 11);
        bi(12, 12); bi(12, 0);

        InternalLog("WFC: Loaded village preset with 13 tiles");
    }

    void RuleSet::LoadCavePreset()
    {
        Clear();

        AddTile(Tile(0, '#', "Wall", COLOR::GRAY, 45.0f));
        AddTile(Tile(1, '.', "Floor", COLOR::GRAY, 35.0f));
        AddTile(Tile(2, '~', "Water", COLOR::BLUE, 5.0f));
        AddTile(Tile(3, '*', "Stalactite", COLOR::WHITE, 3.0f));
        AddTile(Tile(4, 'v', "Stalagmite", COLOR::WHITE, 3.0f));
        AddTile(Tile(5, 'M', "Mushroom", COLOR::MAGENTA, 4.0f));
        AddTile(Tile(6, 'G', "Crystal", COLOR::CYAN, 2.0f));
        AddTile(Tile(7, '>', "Deep", COLOR::BLACK, 2.0f));
        AddTile(Tile(8, 'D', "Deposit", COLOR::YELLOW, 1.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(0, false, false, true, true);
        setProps(2, false, true, true, false);
        setProps(4, false, true, true, false);

        auto addTag = [this](int id, const std::string& tag) {
            Tile* t = GetTileMutable(id);
            if (t) t->AddTag(tag);
            };

        addTag(0, "wall"); addTag(0, "solid");
        addTag(1, "floor"); addTag(1, "walkable");
        addTag(2, "water"); addTag(2, "hazard");
        addTag(3, "decoration"); addTag(3, "ceiling");
        addTag(4, "obstacle");
        addTag(5, "plant"); addTag(5, "walkable");
        addTag(6, "mineral"); addTag(6, "walkable");
        addTag(7, "exit"); addTag(7, "walkable");
        addTag(8, "mineral"); addTag(8, "walkable"); addTag(8, "valuable");

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1); bi(0, 2); bi(0, 3); bi(0, 4); bi(0, 5); bi(0, 6); bi(0, 7); bi(0, 8);
        bi(1, 1); bi(1, 2); bi(1, 4); bi(1, 5); bi(1, 6); bi(1, 7); bi(1, 8);
        bi(2, 2); bi(2, 0);
        bi(3, 3); bi(3, 0);
        bi(4, 4); bi(4, 1);
        bi(5, 5); bi(5, 1);
        bi(6, 6); bi(6, 0); bi(6, 1);
        bi(7, 7); bi(7, 1);
        bi(8, 8); bi(8, 0); bi(8, 1);

        InternalLog("WFC: Loaded cave preset with 9 tiles");
    }

    void RuleSet::LoadIslandPreset()
    {
        Clear();

        AddTile(Tile(0, static_cast<char>(0xB0), "DeepOcean", COLOR::DARK_BLUE, 20.0f));
        AddTile(Tile(1, '~', "Ocean", COLOR::BLUE, 20.0f));
        AddTile(Tile(2, '=', "Shallows", COLOR::CYAN, 10.0f));
        AddTile(Tile(3, ':', "Beach", COLOR::YELLOW, 12.0f));
        AddTile(Tile(4, '.', "Grass", COLOR::GREEN, 15.0f));
        AddTile(Tile(5, 'T', "Palm", COLOR::GREEN, 5.0f));
        AddTile(Tile(6, 'F', "Jungle", COLOR::DARK_GREEN, 8.0f));
        AddTile(Tile(7, '^', "Hill", COLOR::GRAY, 4.0f));
        AddTile(Tile(8, '*', "Peak", COLOR::WHITE, 2.0f));
        AddTile(Tile(9, 'V', "Village", COLOR::YELLOW, 2.0f));
        AddTile(Tile(10, 'R', "Dock", COLOR::BROWN, 2.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(0, false, true, true, false);
        setProps(1, false, true, true, false);
        setProps(2, false, true, true, false);
        setProps(7, false, true, true, false);

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1);
        bi(1, 1); bi(1, 0); bi(1, 2);
        bi(2, 2); bi(2, 1); bi(2, 3); bi(2, 10);
        bi(3, 3); bi(3, 2); bi(3, 4); bi(3, 10);
        bi(4, 4); bi(4, 3); bi(4, 5); bi(4, 6); bi(4, 7); bi(4, 9);
        bi(5, 5); bi(5, 4); bi(5, 6);
        bi(6, 6); bi(6, 4); bi(6, 5); bi(6, 7);
        bi(7, 7); bi(7, 4); bi(7, 6); bi(7, 8);
        bi(8, 8); bi(8, 7);
        bi(9, 9); bi(9, 4); bi(9, 3);
        bi(10, 10); bi(10, 2); bi(10, 3); bi(10, 4);

        InternalLog("WFC: Loaded island preset with 11 tiles");
    }

    void RuleSet::LoadCityPreset()
    {
        Clear();

        AddTile(Tile(0, 'R', "Road", COLOR::GRAY, 25.0f));
        AddTile(Tile(1, '+', "Intersection", COLOR::GRAY, 8.0f));
        AddTile(Tile(2, 'B', "Building", COLOR::YELLOW, 15.0f));
        AddTile(Tile(3, 'P', "Park", COLOR::GREEN, 10.0f));
        AddTile(Tile(4, 'W', "Water", COLOR::BLUE, 5.0f));
        AddTile(Tile(5, 'S', "Sidewalk", COLOR::YELLOW, 12.0f));
        AddTile(Tile(6, 'T', "Tree", COLOR::DARK_GREEN, 8.0f));
        AddTile(Tile(7, 'C', "Plaza", COLOR::CYAN, 3.0f));
        AddTile(Tile(8, 'H', "Highway", COLOR::RED, 3.0f));
        AddTile(Tile(9, 'G', "Garage", COLOR::BROWN, 4.0f));
        AddTile(Tile(10, 'L', "Lot", COLOR::GRAY, 5.0f));
        AddTile(Tile(11, 'F', "Fence", COLOR::YELLOW, 3.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(2, false, false, true, true);
        setProps(4, false, true, true, false);
        setProps(11, false, true, true, false);

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1); bi(0, 5); bi(0, 7); bi(0, 8);
        bi(1, 1); bi(1, 0); bi(1, 5); bi(1, 7); bi(1, 8);
        bi(2, 2); bi(2, 5); bi(2, 9); bi(2, 11);
        bi(3, 3); bi(3, 5); bi(3, 6); bi(3, 11);
        bi(4, 4); bi(4, 5);
        bi(5, 5); bi(5, 0); bi(5, 1); bi(5, 2); bi(5, 3); bi(5, 4); bi(5, 6); bi(5, 7); bi(5, 10); bi(5, 11);
        bi(6, 6); bi(6, 3); bi(6, 5);
        bi(7, 7); bi(7, 0); bi(7, 1); bi(7, 5);
        bi(8, 8); bi(8, 0); bi(8, 1);
        bi(9, 9); bi(9, 2); bi(9, 5); bi(9, 10);
        bi(10, 10); bi(10, 5); bi(10, 9);
        bi(11, 11); bi(11, 2); bi(11, 3); bi(11, 5);

        InternalLog("WFC: Loaded city preset with 12 tiles");
    }

    void RuleSet::LoadMazePreset()
    {
        Clear();

        AddTile(Tile(0, '#', "Wall", COLOR::GRAY, 50.0f));
        AddTile(Tile(1, '.', "Path", COLOR::BLACK, 40.0f));
        AddTile(Tile(2, 'E', "Entrance", COLOR::GREEN, 2.0f));
        AddTile(Tile(3, 'X', "Exit", COLOR::RED, 2.0f));
        AddTile(Tile(4, 'K', "Key", COLOR::YELLOW, 2.0f));
        AddTile(Tile(5, 'L', "Lock", COLOR::BROWN, 1.0f));
        AddTile(Tile(6, 'T', "Trap", COLOR::RED, 2.0f));
        AddTile(Tile(7, '$', "Treasure", COLOR::YELLOW, 1.0f));

        auto setProps = [this](int id, bool walk, bool trans, bool blockMove, bool blockVis) {
            Tile* t = GetTileMutable(id);
            if (t) {
                t->walkable = walk;
                t->transparent = trans;
                t->blocksMovement = blockMove;
                t->blocksVision = blockVis;
            }
            };

        setProps(0, false, false, true, true);
        setProps(5, false, true, true, false);

        auto bi = [this](int a, int b) {
            AddBidirectionalRule(a, Direction::North, b);
            AddBidirectionalRule(a, Direction::South, b);
            AddBidirectionalRule(a, Direction::East, b);
            AddBidirectionalRule(a, Direction::West, b);
            };

        bi(0, 0); bi(0, 1); bi(0, 2); bi(0, 3); bi(0, 4); bi(0, 5); bi(0, 6); bi(0, 7);

        bi(1, 1); bi(1, 2); bi(1, 3); bi(1, 4); bi(1, 6); bi(1, 7);

        bi(2, 2); bi(2, 1);
        bi(3, 3); bi(3, 1);
        bi(4, 4); bi(4, 1);
        bi(5, 5); bi(5, 1);
        bi(6, 6); bi(6, 1);
        bi(7, 7); bi(7, 1);

        InternalLog("WFC: Loaded maze preset with 8 tiles");
    }

    // Texture
    void WaveFunctionCollapse::RenderWithTextures(int screenX, int screenY, int tileSize) const
    {
        if (!initialized) return;

        int w = grid.GetWidth();
        int h = grid.GetHeight();

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const Cell& cell = grid.GetCell(x, y);
                if (!cell.collapsed) continue;

                const Tile* tile = rules.GetTile(cell.selectedTile);
                if (!tile || !tile->HasTexture()) continue;

                const Texture& tex = tile->GetTexture();
                int drawX = screenX + x * tileSize;
                int drawY = screenY + y * tileSize;

                // Blit texture to screen buffer
                for (int ty = 0; ty < tex.height; ++ty)
                {
                    for (int tx = 0; tx < tex.width; ++tx)
                    {
                        unsigned short col = GetTexPixel(tex, tx, ty);
                        if (col != 0) // skip transparent (0) 
                        {
                            DrawPixel(drawX + tx, drawY + ty, PIXEL_SOLID, col);
                        }
                    }
                }
            }
        }
    }

    void WaveFunctionCollapse::DrawTexturedMap(int offsetX, int offsetY, int tileSize) const
    {
        if (!initialized) return;

        RenderWithTextures(offsetX, offsetY, tileSize);
        UpdateScreen();
    }

    std::unordered_set<int>& Tile::GetAllowedNeighbors(Direction dir) noexcept
    {
        switch (dir)
        {
        case Direction::North:     return north;
        case Direction::South:     return south;
        case Direction::East:      return east;
        case Direction::West:      return west;
        case Direction::NorthEast: return northEast;
        case Direction::NorthWest: return northWest;
        case Direction::SouthEast: return southEast;
        case Direction::SouthWest: return southWest;
        default:                   return north;
        }
    }

    // Tile functions
    const std::unordered_set<int>& Tile::GetAllowedNeighbors(Direction dir) const noexcept
    {
        switch (dir)
        {
        case Direction::North:     return north;
        case Direction::South:     return south;
        case Direction::East:      return east;
        case Direction::West:      return west;
        case Direction::NorthEast: return northEast;
        case Direction::NorthWest: return northWest;
        case Direction::SouthEast: return southEast;
        case Direction::SouthWest: return southWest;
        default:                   return north;
        }
    }

    void Tile::AddNeighbor(Direction dir, int tileId)
    {
        switch (dir)
        {
        case Direction::North:     north.insert(tileId); break;
        case Direction::South:     south.insert(tileId); break;
        case Direction::East:      east.insert(tileId); break;
        case Direction::West:      west.insert(tileId); break;
        case Direction::NorthEast: northEast.insert(tileId); break;
        case Direction::NorthWest: northWest.insert(tileId); break;
        case Direction::SouthEast: southEast.insert(tileId); break;
        case Direction::SouthWest: southWest.insert(tileId); break;
        default: break;
        }
    }

    bool Tile::HasTag(const std::string& tag) const noexcept
    {
        return tags.find(tag) != tags.end();
    }

    void Tile::AddTag(const std::string& tag)
    {
        tags.insert(tag);
    }

    // Textures
    void Tile::SetTexture(const Texture& tex)
    {
        texture = tex;
        hasTexture = true;
    }

    const Texture& Tile::GetTexture() const
    {
        return texture;
    }

    // Cell functions
    void Cell::Reset(const std::vector<int>& allTileIds)
    {
        collapsed = false;
        selectedTile = -1;
        possibilities = allTileIds;
        entropy = static_cast<float>(possibilities.size());
    }

    void Cell::Collapse(int tileId) noexcept
    {
        collapsed = true;
        selectedTile = tileId;
        possibilities.clear();
        possibilities.push_back(tileId);
        entropy = 0.0f;
    }

    float Cell::CalculateEntropy(const std::vector<Tile>& tiles, EntropyMode mode) const
    {
        if (collapsed || possibilities.empty())
        {
            return 0.0f;
        }

        if (mode == EntropyMode::Simple)
        {
            return static_cast<float>(possibilities.size());
        }

        float totalWeight = 0.0f;
        for (int tileId : possibilities)
        {
            for (const auto& tile : tiles)
            {
                if (tile.id == tileId)
                {
                    totalWeight += tile.weight;
                    break;
                }
            }
        }

        if (totalWeight <= 0.0f)
        {
            return static_cast<float>(possibilities.size());
        }

        float entropyVal = 0.0f;
        for (int tileId : possibilities)
        {
            for (const auto& tile : tiles)
            {
                if (tile.id == tileId)
                {
                    float p = tile.weight / totalWeight;
                    if (p > 0.0f)
                    {
                        entropyVal -= p * std::log2(p);
                    }
                    break;
                }
            }
        }

        return entropyVal;
    }

    bool Cell::RemovePossibility(int tileId)
    {
        auto it = std::find(possibilities.begin(), possibilities.end(), tileId);
        if (it != possibilities.end())
        {
            possibilities.erase(it);
            return true;
        }
        return false;
    }

    bool Cell::HasPossibility(int tileId) const noexcept
    {
        return std::find(possibilities.begin(), possibilities.end(), tileId) != possibilities.end();
    }

    // RuleSet functions
    int RuleSet::AddTile(Tile tile)
    {
        if (tile.id < 0)
        {
            tile.id = nextId++;
        }
        else
        {
            nextId = std::max(nextId, tile.id + 1);
        }

        idToIndex[tile.id] = tiles.size();
        tiles.push_back(std::move(tile));
        return tiles.back().id;
    }

    bool RuleSet::RemoveTile(int id)
    {
        auto it = idToIndex.find(id);
        if (it == idToIndex.end())
        {
            return false;
        }

        size_t index = it->second;
        tiles.erase(tiles.begin() + index);

        idToIndex.clear();
        for (size_t i = 0; i < tiles.size(); ++i)
        {
            idToIndex[tiles[i].id] = i;
        }

        return true;
    }

    void RuleSet::AddNeighborRule(int fromTile, Direction dir, int toTile)
    {
        Tile* tile = GetTileMutable(fromTile);
        if (tile)
        {
            tile->AddNeighbor(dir, toTile);
        }
    }

    void RuleSet::AddBidirectionalRule(int tileA, Direction dir, int tileB)
    {
        AddNeighborRule(tileA, dir, tileB);
        AddNeighborRule(tileB, GetOppositeDirection(dir), tileA);
    }

    bool RuleSet::CanConnect(int fromTile, Direction dir, int toTile) const
    {
        const Tile* from = GetTile(fromTile);
        if (!from)
        {
            return false;
        }

        const auto& allowed = from->GetAllowedNeighbors(dir);

        if (allowed.empty())
        {
            return true;
        }

        return allowed.find(toTile) != allowed.end();
    }

    bool RuleSet::ValidateRules() const
    {
        for (const auto& tile : tiles)
        {
            for (Direction dir : GetCardinalDirections())
            {
                for (int neighborId : tile.GetAllowedNeighbors(dir))
                {
                    if (!GetTile(neighborId))
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void RuleSet::Clear()
    {
        tiles.clear();
        idToIndex.clear();
        nextId = 0;
    }

    const Tile* RuleSet::GetTile(int id) const
    {
        auto it = idToIndex.find(id);
        if (it != idToIndex.end())
        {
            return &tiles[it->second];
        }
        return nullptr;
    }

    Tile* RuleSet::GetTileMutable(int id)
    {
        auto it = idToIndex.find(id);
        if (it != idToIndex.end())
        {
            return &tiles[it->second];
        }
        return nullptr;
    }

    std::vector<int> RuleSet::GetTileIds() const
    {
        std::vector<int> ids;
        ids.reserve(tiles.size());
        for (const auto& tile : tiles)
        {
            ids.push_back(tile.id);
        }
        return ids;
    }

    float RuleSet::GetTotalWeight() const
    {
        float total = 0.0f;
        for (const auto& tile : tiles)
        {
            total += tile.weight;
        }
        return total;
    }

    float RuleSet::GetTileWeight(int id) const
    {
        const Tile* tile = GetTile(id);
        return tile ? tile->weight : 0.0f;
    }

    const Tile* RuleSet::FindTileBySymbol(char symbol) const
    {
        for (const auto& tile : tiles)
        {
            if (tile.symbol == symbol)
            {
                return &tile;
            }
        }
        return nullptr;
    }

    const Tile* RuleSet::FindTileByName(const std::string& name) const
    {
        for (const auto& tile : tiles)
        {
            if (tile.name == name)
            {
                return &tile;
            }
        }
        return nullptr;
    }

    std::vector<const Tile*> RuleSet::GetTilesWithTag(const std::string& tag) const
    {
        std::vector<const Tile*> result;
        for (const auto& tile : tiles)
        {
            if (tile.HasTag(tag))
            {
                result.push_back(&tile);
            }
        }
        return result;
    }

    void RuleSet::LoadPreset(PresetType preset)
    {
        // Each loader clears the rule set itself, so no Clear() here.
        switch (preset)
        {
        case PresetType::Terrain: LoadTerrainPreset(); break;
        case PresetType::Dungeon: LoadDungeonPreset(); break;
        case PresetType::Village: LoadVillagePreset(); break;
        case PresetType::Cave:    LoadCavePreset();    break;
        case PresetType::Island:  LoadIslandPreset();  break;
        case PresetType::City:    LoadCityPreset();    break;
        case PresetType::Maze:    LoadMazePreset();    break;
        default:                  Clear();             break;
        }
    }

    // Texture
    void RuleSet::SetTileTexture(int tileId, const Texture& tex)
    {
        Tile* tile = GetTileMutable(tileId);
        if (tile)
        {
            tile->SetTexture(tex);
        }
    }

    bool RuleSet::LoadTileTexture(int tileId, const std::wstring& path)
    {
        Tile* tile = GetTileMutable(tileId);
        if (!tile) return false;

        // Must be value-initialised: LoadTexture() deletes outTex.pixels before
        // writing, so an indeterminate pointer here is a free() of garbage.
        Texture tex{};
        if (LoadTexture(path, tex))
        {
            tile->SetTexture(tex);
            return true;
        }
        return false;
    }

    // Grid functions
    void Grid::Initialize(int w, int h, const std::vector<int>& tileIds)
    {
        width = w;
        height = h;
        cells.clear();
        cells.reserve(static_cast<size_t>(w) * h);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                Cell cell({ static_cast<float>(x), static_cast<float>(y) });
                cell.Reset(tileIds);
                cells.push_back(std::move(cell));
            }
        }
    }

    void Grid::Reset(const std::vector<int>& tileIds)
    {
        for (auto& cell : cells)
        {
            cell.Reset(tileIds);
        }
    }

    void Grid::Resize(int w, int h, const std::vector<int>& tileIds)
    {
        Initialize(w, h, tileIds);
    }

    Cell& Grid::GetCell(int x, int y)
    {
        return cells[GetIndex(x, y)];
    }

    const Cell& Grid::GetCell(int x, int y) const
    {
        return cells[GetIndex(x, y)];
    }

    Cell& Grid::GetCellByIndex(int index)
    {
        return cells[index];
    }

    const Cell& Grid::GetCellByIndex(int index) const
    {
        return cells[index];
    }

    std::vector<std::pair<Cell*, Direction>> Grid::GetNeighbors(int x, int y, bool includeDiagonals)
    {
        std::vector<std::pair<Cell*, Direction>> neighbors;
        neighbors.reserve(includeDiagonals ? 8 : 4);

        auto directions = includeDiagonals ? GetAllDirections() : GetCardinalDirections();

        for (Direction dir : directions)
        {
            Vector2 offset = GetDirectionOffset(dir);
            int nx = x + static_cast<int>(offset.x);
            int ny = y + static_cast<int>(offset.y);

            if (InsideBounds(nx, ny))
            {
                neighbors.emplace_back(&GetCell(nx, ny), dir);
            }
        }

        return neighbors;
    }

    std::vector<std::pair<const Cell*, Direction>> Grid::GetNeighbors(int x, int y, bool includeDiagonals) const
    {
        std::vector<std::pair<const Cell*, Direction>> neighbors;
        neighbors.reserve(includeDiagonals ? 8 : 4);

        auto directions = includeDiagonals ? GetAllDirections() : GetCardinalDirections();

        for (Direction dir : directions)
        {
            Vector2 offset = GetDirectionOffset(dir);
            int nx = x + static_cast<int>(offset.x);
            int ny = y + static_cast<int>(offset.y);

            if (InsideBounds(nx, ny))
            {
                neighbors.emplace_back(&GetCell(nx, ny), dir);
            }
        }

        return neighbors;
    }

    int Grid::GetIndex(int x, int y) const noexcept
    {
        return y * width + x;
    }

    Vector2 Grid::GetPosition(int index) const noexcept
    {
        return { static_cast<float>(index % width), static_cast<float>(index / width) };
    }

    bool Grid::InsideBounds(int x, int y) const noexcept
    {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    size_t Grid::CountCollapsed() const
    {
        size_t count = 0;
        for (const auto& cell : cells)
        {
            if (cell.collapsed)
            {
                ++count;
            }
        }
        return count;
    }

    size_t Grid::CountContradictions() const
    {
        size_t count = 0;
        for (const auto& cell : cells)
        {
            if (cell.IsContradiction())
            {
                ++count;
            }
        }
        return count;
    }

    bool Grid::IsFullyCollapsed() const
    {
        for (const auto& cell : cells)
        {
            if (!cell.collapsed)
            {
                return false;
            }
        }
        return true;
    }

    bool Grid::HasContradiction() const
    {
        for (const auto& cell : cells)
        {
            if (cell.IsContradiction())
            {
                return true;
            }
        }
        return false;
    }

}