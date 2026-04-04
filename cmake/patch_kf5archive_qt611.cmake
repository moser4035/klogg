function(klogg_patch_file file_path original_text replacement_text)
  file(READ "${file_path}" file_contents)
  string(FIND "${file_contents}" "${replacement_text}" replacement_pos)
  if(NOT replacement_pos EQUAL -1)
    return()
  endif()

  string(FIND "${file_contents}" "${original_text}" original_pos)
  if(original_pos EQUAL -1)
    message(FATAL_ERROR "Could not patch ${file_path}: expected text not found")
  endif()

  string(REPLACE "${original_text}" "${replacement_text}" file_contents "${file_contents}")
  file(WRITE "${file_path}" "${file_contents}")
endfunction()

function(klogg_patch_kf5archive_for_qt6_11 source_dir)
  klogg_patch_file(
    "${source_dir}/karchive/src/karchive.cpp"
    "        setErrorString(tr(\"Could not set device mode to %1\").arg(mode));"
    "        setErrorString(tr(\"Could not set device mode to %1\").arg(static_cast<int>(mode)));"
  )

  klogg_patch_file(
    "${source_dir}/karchive/src/karchive.cpp"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(d->mode));"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(static_cast<int>(d->mode)));"
  )

  klogg_patch_file(
    "${source_dir}/karchive/src/kar.cpp"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(mode));"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(static_cast<int>(mode)));"
  )

  klogg_patch_file(
    "${source_dir}/karchive/src/krcc.cpp"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(mode));"
    "        setErrorString(tr(\"Unsupported mode %1\").arg(static_cast<int>(mode)));"
  )
endfunction()
