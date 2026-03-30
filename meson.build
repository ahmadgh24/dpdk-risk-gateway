project('dpdk-risk-gateway', 'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++17', 
    'optimization=3', 
    'warning_level=3'
  ]
)

# 1. Find DPDK via pkg-config
dpdk = dependency('libdpdk')

# 2. Define include directories
inc = include_directories('include')

# 3. Source files
sources = files(
  'src/main.cpp',
  'src/RiskEngine.cpp'
)

# 4. Build the executable
executable('riskgw',
  sources,
  include_directories: inc,
  dependencies: dpdk,
  install: true,
  # This ensures the compiler uses the best instructions for your CPU
  cpp_args: ['-march=native', '-DALLOW_EXPERIMENTAL_API']
)
