# Unified Search Architecture - Deployment Guide

**Date:** 2026-02-12  
**Branch:** feature/unified-search-architecture  
**Status:** Ready for Deployment

---

## Overview

This guide provides step-by-step instructions for deploying the unified search architecture to production. It covers pre-deployment checks, deployment procedures, monitoring, and rollback procedures.

---

## Prerequisites

### System Requirements

- **aMule Version:** Latest development version
- **C++ Compiler:** C++17 compatible (GCC 7+, Clang 5+, MSVC 2017+)
- **wxWidgets:** 3.0+
- **CMake:** 3.10+
- **Google Test:** 1.10+ (for testing)
- **RAM:** Minimum 2GB (4GB recommended)
- **Storage:** 100MB additional disk space

### Pre-Deployment Checklist

- [ ] All unit tests passing (80+ tests)
- [ ] All integration tests passing (35+ tests)
- [ ] Performance benchmarks completed
- [ ] Load testing completed
- [ ] Memory leak detection completed
- [ ] Code review completed
- [ ] Documentation reviewed
- [ ] Feature flags configured
- [ ] Migration plan approved
- [ ] Rollback plan tested
- [ ] Monitoring tools configured

---

## Deployment Strategy

### Gradual Rollout Approach

The unified search architecture supports gradual rollout through feature flags. This allows for:

1. **Safe Deployment:** Deploy code without enabling features
2. **Beta Testing:** Enable features for subset of users
3. **Monitoring:** Monitor for issues before full rollout
4. **Quick Rollback:** Disable features if issues discovered

### Rollout Phases

**Phase 1: Code Deployment (Feature Flags Disabled)**
- Deploy new code to all users
- All feature flags disabled
- No behavior changes
- Verify deployment stability

**Phase 2: Beta Testing (10% of Users)**
- Enable UNIFIED_SEARCH_MANAGER for 10% of users
- Enable UNIFIED_LOCAL_SEARCH for 10% of users
- Monitor for issues
- Collect feedback

**Phase 3: Expanded Beta (50% of Users)**
- Enable additional features for 50% of users
- Enable UNIFIED_GLOBAL_SEARCH
- Monitor performance
- Collect feedback

**Phase 4: Full Rollout (100% of Users)**
- Enable all features for all users
- Enable UNIFIED_KAD_SEARCH
- Enable UNIFIED_SEARCH_UI
- Monitor production

**Phase 5: Cleanup**
- Remove old search code
- Remove feature flags
- Update documentation

---

## Deployment Procedures

### Step 1: Prepare Environment

```bash
# Clone or update repository
git clone https://github.com/3togo/amule.git
cd amule
git checkout feature/unified-search-architecture
git pull origin feature/unified-search-architecture

# Create build directory
mkdir build && cd build
```

### Step 2: Build with Unified Search

```bash
# Configure CMake
cmake .. -DENABLE_UNIFIED_SEARCH=ON -DBUILD_TESTS=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Step 3: Configure Feature Flags

Create feature flags configuration file:

```bash
# Create feature flags file
cat > ~/.amule/feature_flags.conf << EOF
# aMule Feature Flags
UNIFIED_SEARCH_MANAGER=false
UNIFIED_LOCAL_SEARCH=false
UNIFIED_GLOBAL_SEARCH=false
UNIFIED_KAD_SEARCH=false
UNIFIED_SEARCH_UI=false
EOF
```

### Step 4: Deploy Code

```bash
# Stop aMule service
systemctl stop amule

# Backup existing installation
cp /usr/local/bin/amule /usr/local/bin/amule.backup

# Install new version
sudo make install

# Start aMule service
systemctl start amule

# Verify deployment
amule --version
```

### Step 5: Verify Deployment

1. **Check Logs:**
   ```bash
   tail -f ~/.amule/amule.log
   ```

2. **Verify Feature Flags:**
   ```bash
   # Check feature flags are loaded
   grep "Feature flags" ~/.amule/amule.log
   ```

3. **Test Basic Functionality:**
   - Launch aMule
   - Navigate to Search tab
   - Verify old search still works (feature flags disabled)
   - Check for errors in logs

---

## Monitoring

### Key Metrics to Monitor

1. **Search Performance**
   - Average search latency
   - Search throughput (searches/second)
   - Result processing rate (results/second)

2. **Resource Usage**
   - Memory usage (should not increase significantly)
   - CPU usage during searches
   - Thread count

3. **Error Rates**
   - Search failures
   - Timeouts
   - Errors in logs

4. **User Experience**
   - Search results quality
   - UI responsiveness
   - User feedback

### Monitoring Commands

```bash
# Check memory usage
ps aux | grep amule | awk '{print $6}'

# Check CPU usage
top -b -n 1 | grep amule

# Check thread count
ps -eLf | grep amule | wc -l

# Monitor logs for errors
grep -i error ~/.amule/amule.log | tail -20

# Monitor search statistics
grep "UnifiedSearchManager" ~/.amule/amule.log | tail -20
```

### Log Analysis

**Successful Search:**
```
[UnifiedSearchManager] Starting local search 1234567890 for query: music
[LocalSearchEngine] Search started successfully
[UnifiedSearchManager] Search completed: 1234567890
```

**Search Error:**
```
[UnifiedSearchManager] Error starting search: Invalid parameters
```

**Feature Flag Change:**
```
[FeatureFlags] Feature UNIFIED_LOCAL_SEARCH enabled
```

---

## Rollback Procedures

### Immediate Rollback (Feature Flags)

If issues are discovered:

```bash
# Disable all unified search features
cat > ~/.amule/feature_flags.conf << EOF
# aMule Feature Flags
UNIFIED_SEARCH_MANAGER=false
UNIFIED_LOCAL_SEARCH=false
UNIFIED_GLOBAL_SEARCH=false
UNIFIED_KAD_SEARCH=false
UNIFIED_SEARCH_UI=false
EOF

# Restart aMule
systemctl restart amule
```

### Automated Rollback

```bash
# Get migration report
# (Implementation would provide this)

# Perform rollback
# (Implementation would provide this)

# Verify rollback
amule --version
# Should show old version or features disabled
```

### Code Rollback

If critical issues require code rollback:

```bash
# Stop aMule
systemctl stop amule

# Restore backup
sudo cp /usr/local/bin/amule.backup /usr/local/bin/amule

# Start aMule
systemctl start amule

# Verify
amule --version
```

### Rollback Verification

After rollback:

1. Verify old search system works correctly
2. Verify no data loss
3. Verify no search result corruption
4. Check logs for errors
5. Monitor resource usage

---

## Troubleshooting

### Issue: Search Not Starting

**Symptoms:** Search command sent but no events received

**Possible Causes:**
- Feature flag not enabled
- UnifiedSearchManager not initialized
- Worker thread crashed

**Solutions:**
```bash
# Check feature flags
grep UNIFIED ~/.amule/feature_flags.conf

# Check logs
grep "UnifiedSearchManager" ~/.amule/amule.log

# Restart aMule
systemctl restart amule
```

### Issue: Memory Leak Detected

**Symptoms:** Memory usage increases over time

**Possible Causes:**
- Search results not cleaned up
- Event callbacks not unregistered
- Worker thread not shutting down

**Solutions:**
```bash
# Check memory usage
watch -n 1 'ps aux | grep amule | awk "{print \$6}"'

# Force cleanup
# Send maintenance command
# (Implementation would provide)

# Restart aMule
systemctl restart amule
```

### Issue: High CPU Usage

**Symptoms:** CPU usage spikes during searches

**Possible Causes:**
- Too many concurrent searches
- Inefficient result processing
- Serialization overhead

**Solutions:**
```bash
# Check CPU usage
top -b -n 1 | grep amule

# Reduce concurrent searches
# Configure maxConcurrentSearches

# Restart aMule
systemctl restart amule
```

### Issue: Search Results Not Displayed

**Symptoms:** Search completes but no results appear

**Possible Causes:**
- Result callback not registered
- UI thread blocked
- Event routing failure

**Solutions:**
```bash
# Check logs for events
grep "RESULTS_RECEIVED" ~/.amule/amule.log

# Check UI thread
# Monitor for UI thread blocking

# Restart aMule
systemctl restart amule
```

---

## Performance Optimization

### Configuration Tuning

**UnifiedSearchManager:**
```cpp
struct Config {
    std::chrono::milliseconds maintenanceInterval{1000};  // Reduce for faster cleanup
    std::chrono::milliseconds commandTimeout{5000};
    size_t maxConcurrentSearches{10};  // Increase for better throughput
    size_t maxResultsPerSearch{500};
};
```

**LocalSearchEngine:**
```cpp
// Optimize result filtering
params.enableResultDeduplication = true;
params.maxResults = 100;  // Reduce for faster processing
```

**GlobalSearchEngine:**
```cpp
// Optimize server selection
config.maxServersPerSearch = 5;  // Reduce for faster response
config.requestTimeout = std::chrono::milliseconds(15000);
```

**KadSearchEngine:**
```cpp
// Optimize node selection
config.maxConcurrentRequests = 5;  // Reduce for faster response
config.jumpStartInterval = std::chrono::milliseconds(10000);
```

### Monitoring Performance

```bash
# Profile search latency
time amule --search "test query"

# Profile result processing
grep "Results received" ~/.amule/amule.log | wc -l

# Profile memory usage
valgrind --tool=massif amule
```

---

## Post-Deployment Checklist

- [ ] All feature flags configured correctly
- [ ] Monitoring tools configured
- [ ] Alert thresholds set
- [ ] Rollback procedures tested
- [ ] Documentation updated
- [ ] Support team trained
- [ ] User communication sent
- [ ] Performance baseline established
- [ ] Error tracking configured
- [ ] Log rotation configured

---

## Support and Contact

For issues or questions:

1. Check this deployment guide
2. Review the main README
3. Consult the architecture design document
4. Check integration tests for examples
5. Contact development team

---

## Appendix: Feature Flag Reference

| Flag Name | Default | Description |
|-----------|---------|-------------|
| `UNIFIED_SEARCH_MANAGER` | false | Enable unified search manager |
| `UNIFIED_LOCAL_SEARCH` | false | Enable unified local search |
| `UNIFIED_GLOBAL_SEARCH` | false | Enable unified global search |
| `UNIFIED_KAD_SEARCH` | false | Enable unified Kad search |
| `UNIFIED_SEARCH_UI` | false | Enable unified search UI |

---

**Document Version:** 1.0  
**Last Updated:** 2026-02-12  
**Related:** [README.md](README.md), [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md), [FINAL_IMPLEMENTATION_SUMMARY.md](FINAL_IMPLEMENTATION_SUMMARY.md)
