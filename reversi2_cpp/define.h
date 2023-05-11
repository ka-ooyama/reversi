#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS 6  // �c�̃}�X��
#define ROWS 6    // ���̃}�X��

// ���萔 = COLUMNS * ROWS - 4(�����z�u)
#define TURNS (COLUMNS * ROWS - 4)

// �i6x6���Ō�܂ŉ�͂���͎̂��Ԃ������肷����̂Łj�����̉�͂ȂǓr���Ő؂�グ�����Ƃ���
#if true  // ������ true �ɂ��Ď��s�������K�w��������
#undef TURNS
#define TURNS 16
static_assert((TURNS < COLUMNS* ROWS - 4), "TURNS is too large.");
#endif

// �i�擪����j�V���O���X���b�h�ŏ�������K�w��
#define SINGLE_HIERARCHEY_TOP 0
// �i���[����j�V���O���X���b�h�ŏ�������K�w��
#define SINGLE_HIERARCHEY_BTM 10

// �L���b�V����L��������K�w�̐[��
// �}���藦���Ⴏ��Ό����̃R�X�g�̂ق��������t���i6x6�Ō������L���ɂ��Ȃ��ق��������j
#define CACHED_HIERARCHEY 15

// ���s���J��Ԃ���
// 4x4�ȂǑ������Đ��m�Ɍv���ł��Ȃ��Ƃ�������܂킵�ĕ��ς��Ƃ�
#define NUMBER_OF_TRIALS 1

// �g�p����X���b�h�̐��C�傫�Ȓl���w�肵�Ă��g�p���Ă���CPU�̍ő�l�ȏ�ɂ͂Ȃ�Ȃ�
#define WORKER_THREAD_MAX 20

// �A���t�@�x�[�^�@�Ŏ}���肷��
#define OPT_ALPHA_BETA true
// �s�Ɨ񂪋����������Ƃ��P��ڂ��ȗ�����i�S���������ׂĂ��������ʂɂȂ邽�߁j
// 4x4, 4x6, 4x8, 4x10 ���r����ۂȂǂ� false �ɂ���i�܂���4x4�̌��ʂ�4�{���Ĕ�r����K�v������j
#define OPT_TETRAGONALITY true

// 6x6�����̃��[�u�I�[�_�����O�e�[�u�����g��
#define OPT_MOVE_ORDERING_6x6 true
// �P��ڂɁi��͍ς݂́j�őP���łi4x4,4x6,4x8,6x6�̂ݑΉ��j
#define OPT_MOVE_ORDERING_ANSER true

// �v���ɉe�����o��悤�ȃf�o�b�O�\�����������߂̒�`
// ���i��true�ɂ��Ă������ق����m�F���₷������ǂ��ŏI�I�Ȍv����false�ōs��
#define DEBUG_PRINT true
// �i�}���肳�ꂸ�Ɂj�������ꂽ�Ֆʂ̐�
#define PRINT_NODES false

// ���ʂ��L���b�V������
#define OPT_CACHE false
// �L���b�V�������ɑΏ̐��𗘗p����i8�p�^�[���𓯂��Ƃ݂Ȃ����ƂŃL���b�V��������������j
#define OPT_SYMMETRY false
// �L���b�V���̃q�b�g����\������
#define CACHE_ANALYZE true

#endif  // DEFINE_H
