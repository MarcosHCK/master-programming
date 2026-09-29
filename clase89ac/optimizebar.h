/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <indicators/dynamic_progress.hpp>

class optimize_bar
{

  static inline auto make_progress_bar (size_t generations)
    {

      return indicators::BlockProgressBar {

        indicators::option::BarWidth { 80 },
        indicators::option::ForegroundColor { indicators::Color::white },
        indicators::option::ShowPercentage { true },
        indicators::option::FontStyles { std::vector { indicators::FontStyle::bold } },
        indicators::option::MaxProgress { generations },
      };
    }

  indicators::BlockProgressBar _progress_bar;
  indicators::DynamicProgress<indicators::BlockProgressBar> _bar;

public:

  inline optimize_bar (size_t generations):
      _progress_bar (make_progress_bar (generations)), _bar (*&_progress_bar)
    {
      _bar.set_option (indicators::option::HideBarWhenComplete { true });
      indicators::show_console_cursor (false);
    }

  inline ~optimize_bar ()
    {
      _bar [0].mark_as_completed ();
      _bar.print_progress ();
      indicators::show_console_cursor (true);
    }

  inline void operator() (int g) noexcept
    {
      _bar [0].tick ();
    }
};