/*
 * texto.c
 * Funções para implementação de editor de texto.
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014, 2015 João V. F. Lima, UFSM
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "texto.h"
#include "tela.h"
#include "memo.h"
#include "lista.h"

/* estados para o campo texto */
enum { nada, editando } estado;

texto_t* texto_inicia(char* arq)
{

	texto_t* t = (texto_t*)memo_aloca(sizeof(texto_t));
	tamanho_t tam = { 600, 400 };	/* tamanho da tela */
	
	tela_inicializa(&t->tela, tam, "Outro Editor");
	tela_limpa(&t->tela);

	t->nome = arq;
	t->nlin = 0;
	t->lincur = 0;
	t->colcur = 0;
	t->lin1 = 0;
	t->col1 = 0;

	/* inicialize aqui quaisquer outras estruturas, como a lista de linhas */

	return t;
}

void texto_destroi(texto_t* txt)
{
	tela_limpa(&txt->tela);
	tela_finaliza(&txt->tela);
	while(txt->linhas->n > 0)
		txt->linhas = list_remove(txt->linhas, txt->linhas->n);
	list_destroy(txt->linhas);
	memo_libera(txt);
}

tela_t* texto_tela(texto_t* txt)
{
	assert( txt != NULL );
	return &txt->tela;
}

void texto_desenha_cursor_tela(texto_t *txt)
{
	cor_t preto = {0.0, 0.0, 0.0};
	tamanho_t tt;
	ponto_t pt1, pt2;

	{
		line* ln = list_search(txt->linhas->first, txt->lincur);
		char* texto =  ln->text;
		char subtexto[60];
		strncpy(subtexto, texto, txt->colcur*sizeof(char));
		subtexto[txt->colcur] = '\0';
		tt = tela_tamanho_texto(&txt->tela, subtexto);
	}
	/* posicao x (horizontal) do cursor */
	pt1.x = tt.larg + 1;
	/* posicao y (vertical) do cursor */
	pt1.y = txt->lincur * tt.alt;
	pt2.x = pt1.x;
	pt2.y = pt1.y + tt.alt;
	tela_cor(&txt->tela, preto);
	tela_linha(&txt->tela, pt1, pt2);
}

void texto_desenha_tela(texto_t *txt)
{
	cor_t cor;
	tamanho_t tt;
	ponto_t pt;
	int i;
	line* ln;

	cor.r = cor.g = cor.b = (float)0;
	
	/* limpa a tela. Comentar se ficar lento */
	tela_limpa(&txt->tela);
	
	for(i = 1; i <= txt->nlin+1; i++){
		ln = list_search(txt->linhas->first, i-1);
		tt = tela_tamanho_texto(&txt->tela, ln->text);

		/* calcula posicao da nova linha */
		pt.x = 1;
		pt.y = (i - 1)*tt.alt + 1;

		/* muda cor e desenha linha */
		tela_cor(&txt->tela, cor);
		tela_texto(&txt->tela, pt, ln->text);
	}

	texto_desenha_cursor_tela(txt);
}

void texto_atualiza_tela(texto_t *txt)
{
	texto_desenha_tela(txt);
	tela_mostra(texto_tela(txt));
	tela_espera(30);
}

bool texto_processa_comandos(texto_t* txt)
{
	int tecla = tela_tecla(texto_tela(txt));
	int modificador = tela_tecla_modificador(texto_tela(txt));
	/* apertou CRTL + Q ? */

	if( tecla == ALLEGRO_EVENT_DISPLAY_CLOSE ){
		printf("CLOSE\n");
		return false;
	}
	if( tecla == ALLEGRO_KEY_Q && (modificador & ALLEGRO_KEYMOD_CTRL) ) {
		printf("CTRL+Q SAIR\n");
		return false;
	}
	if( tecla == ALLEGRO_KEY_S && (modificador & ALLEGRO_KEYMOD_CTRL) ) {
		printf("CTRL+S SALVAR e PARAR DE EDITAR\n");
		/* muda estado na variável para não editar */
		estado = nada;
	}
	if( tecla == ALLEGRO_KEY_E && (modificador & ALLEGRO_KEYMOD_CTRL) ) {
		printf("CTRL+E EDITAR\n");
		/* muda estado na variável para editando */
		estado = editando;
	}

	/* teclas direcionais 
		ALLEGRO_KEY_LEFT
		ALLEGRO_KEY_RIGHT
		ALLEGRO_KEY_UP
		ALLEGRO_KEY_DOWN
	*/
	if( tecla == ALLEGRO_KEY_LEFT )
		texto_move_esq(txt);
	if( tecla == ALLEGRO_KEY_RIGHT )
		texto_move_dir(txt);
	if( tecla == ALLEGRO_KEY_UP )
		texto_move_cima(txt);
	if( tecla == ALLEGRO_KEY_DOWN )
		texto_move_baixo(txt);

	return true;
}

void texto_le_arquivo(texto_t *txt, char *nome, FILE *arq)
{

	char c;
	int i, j;
	line* ln;

	i = j = 0;

	c = fgetc(arq);

	while(feof(arq)==0){
		txt->linhas = list_insert(txt->linhas, i);
		ln = list_search(txt->linhas->first, i);

		do{
			ln->text = memo_realoca(ln->text, strlen(ln->text)+sizeof(char));
			ln->text[j] = c;
			j++;
			c = fgetc(arq);
		}while(c!='\n' && feof(arq)==0);
		ln->text[j] = '\0';
		j = 0;
		i++;
		c = fgetc(arq);
	}
	txt->nlin = i-1;
}

void texto_move_esq(texto_t *txt)
{
	if(txt->colcur == 0 && txt->lincur > 0){
		txt->lincur--;
		txt->colcur = strlen(list_search(txt->linhas->first, txt->lincur)->text);
	}else if(txt->colcur > 0){
		txt->colcur--;
	}
}

void texto_move_dir(texto_t *txt)
{	
	int size = strlen(list_search(txt->linhas->first, txt->lincur)->text);
	if(txt->colcur == size && txt->lincur < txt->nlin){
		txt->lincur++;
		txt->colcur = 0;
	}else if(txt->colcur < size && txt->lincur <= txt->nlin){
		txt->colcur++;
	}
}

void texto_move_baixo(texto_t *txt)
{
	if(txt->lincur <= txt->nlin-1)
		txt->lincur++;
}

void texto_move_cima(texto_t *txt)
{
	if(txt->lincur > 0)
		txt->lincur--;
}

