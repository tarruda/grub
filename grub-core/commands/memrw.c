/* memrw.c - command to read / write physical memory  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/env.h>
#include <grub/i18n.h>
#include <grub/lua.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_extcmd_t cmd_read_byte, cmd_read_word, cmd_read_dword;
static grub_command_t cmd_write_byte, cmd_write_word, cmd_write_dword;
static grub_command_t cmd_write_bytes;

static const struct grub_arg_option options[] =
  {
    {0, 'v', 0, N_("Save read value into variable VARNAME."),
     N_("VARNAME"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };


static grub_err_t
grub_cmd_read (grub_extcmd_context_t ctxt, int argc, char **argv)
{
  grub_addr_t addr;
  grub_uint32_t value = 0;
  char buf[sizeof ("XXXXXXXX")];

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  addr = grub_strtoul (argv[0], 0, 0);
  switch (ctxt->extcmd->cmd->name[sizeof ("read_") - 1])
    {
    case 'd':
      value = *((volatile grub_uint32_t *) addr);
      break;

    case 'w':
      value = *((volatile grub_uint16_t *) addr);
      break;

    case 'b':
      value = *((volatile grub_uint8_t *) addr);
      break;
    }

  if (ctxt->state[0].set)
    {
      grub_snprintf (buf, sizeof (buf), "%x", value);
      grub_env_set (ctxt->state[0].arg, buf);
    }
  else
    grub_printf ("0x%x\n", value);

  return 0;
}

static grub_err_t
grub_cmd_write (grub_command_t cmd, int argc, char **argv)
{
  grub_addr_t addr;
  grub_uint32_t value;
  grub_uint32_t mask = 0xffffffff;

  if (argc != 2 && argc != 3)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  addr = grub_strtoul (argv[0], 0, 0);
  value = grub_strtoul (argv[1], 0, 0);
  if (argc == 3)
    mask = grub_strtoul (argv[2], 0, 0);
  value &= mask;
  switch (cmd->name[sizeof ("write_") - 1])
    {
    case 'd':
      if (mask != 0xffffffff)
	*((volatile grub_uint32_t *) addr)
	  = (*((volatile grub_uint32_t *) addr) & ~mask) | value;
      else
	*((volatile grub_uint32_t *) addr) = value;
      break;

    case 'w':
      if ((mask & 0xffff) != 0xffff)
	*((volatile grub_uint16_t *) addr)
	  = (*((volatile grub_uint16_t *) addr) & ~mask) | value;
      else
	*((volatile grub_uint16_t *) addr) = value;
      break;

    case 'b':
      if ((mask & 0xff) != 0xff)
	*((volatile grub_uint8_t *) addr)
	  = (*((volatile grub_uint8_t *) addr) & ~mask) | value;
      else
	*((volatile grub_uint8_t *) addr) = value;
      break;
    }

  return 0;
}

static grub_err_t
grub_cmd_write_bytes (grub_command_t cmd __attribute__ ((unused)),
  int argc, char **argv)
{
  grub_addr_t addr;
  grub_uint32_t value;

  if (argc < 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
      N_("at least two arguments expected"));

  addr = grub_strtoul (argv[0], 0, 0);
  for (int i = 1; i < argc; i++)
    {
      value = grub_strtoul (argv[i], 0, 0) & 0xFF;
      *((volatile grub_uint8_t *) addr + (i-1)) = value;
    }

  return 0;
}

static int
lua_read_byte (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  lua_pushinteger (state, *((grub_uint8_t *) addr));
  return 1;
}

static int
lua_read_word (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  lua_pushinteger (state, *((grub_uint16_t *) addr));
  return 1;
}

static int
lua_read_dword (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  lua_pushinteger (state, *((grub_uint32_t *) addr));
  return 1;
}

static int
lua_write_byte (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  *((grub_uint8_t *) addr) = luaL_checkinteger (state, 2);
  return 1;
}

static int
lua_write_word (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  *((grub_uint16_t *) addr) = luaL_checkinteger (state, 2);
  return 1;
}

static int
lua_write_dword (lua_State *state)
{
  grub_addr_t addr;
  addr = luaL_checkinteger (state, 1);
  *((grub_uint32_t *) addr) = luaL_checkinteger (state, 2);
  return 1;
}

static luaL_Reg memlib[] =
{
  {"read_byte", lua_read_byte},
  {"read_word", lua_read_word},
  {"read_dword", lua_read_dword},
  {"write_byte", lua_write_byte},
  {"write_word", lua_write_word},
  {"write_dword", lua_write_dword},
  {0, 0}
};

GRUB_MOD_INIT(memrw)
{
  cmd_read_byte =
    grub_register_extcmd ("read_byte", grub_cmd_read, 0,
			  N_("ADDR"), N_("Read 8-bit value from ADDR."),
			  options);
  cmd_read_word =
    grub_register_extcmd ("read_word", grub_cmd_read, 0,
			  N_("ADDR"), N_("Read 16-bit value from ADDR."),
			  options);
  cmd_read_dword =
    grub_register_extcmd ("read_dword", grub_cmd_read, 0,
			  N_("ADDR"), N_("Read 32-bit value from ADDR."),
			  options);
  cmd_write_byte =
    grub_register_command ("write_byte", grub_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 8-bit VALUE to ADDR."));
  cmd_write_word =
    grub_register_command ("write_word", grub_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 16-bit VALUE to ADDR."));
  cmd_write_dword =
    grub_register_command ("write_dword", grub_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 32-bit VALUE to ADDR."));
  cmd_write_bytes =
    grub_register_command ("write_bytes", grub_cmd_write_bytes,
			   N_("ADDR VALUE1 [VALUE2 [VALUE3 ...]]"),
			   N_("Write sequences of 8-bit VALUES to ADDR."));
  if (grub_lua_global_state)
  {
    lua_gc (grub_lua_global_state, LUA_GCSTOP, 0);
    luaL_register (grub_lua_global_state, "memrw", memlib);
    lua_gc (grub_lua_global_state, LUA_GCRESTART, 0);
  }
}

GRUB_MOD_FINI(memrw)
{
  grub_unregister_extcmd (cmd_read_byte);
  grub_unregister_extcmd (cmd_read_word);
  grub_unregister_extcmd (cmd_read_dword);
  grub_unregister_command (cmd_write_byte);
  grub_unregister_command (cmd_write_word);
  grub_unregister_command (cmd_write_dword);
  grub_unregister_command (cmd_write_bytes);
}
