/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mchiaram <mchiaram@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 14:06:58 by mchiaram          #+#    #+#             */
/*   Updated: 2025/12/01 14:06:59 by mchiaram         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Bot.hpp"

int	main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cerr << "Not enough arguments" << std::endl;
		exit(EXIT_FAILURE);
	}

	Bot	bot(argv);

	bot.run();
}